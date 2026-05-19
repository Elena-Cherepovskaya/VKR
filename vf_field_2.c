
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/mem.h"
#include "avfilter.h"
#include "filters.h"

typedef double custom_type;

typedef struct Field_2_Context
{
    const AVClass *class;

    int h;
    int w;
    int frame_num;
    int block_size;
    int fps;


    int mask_width;
    int mask_height;
    int mask_size;
    uint8_t *mask;


    custom_type *C;
    custom_type *CT;
    custom_type *dct_temp;
    custom_type *dct_block;


    custom_type *sin_x_table_even;
    custom_type *sin_y_table_even;
    custom_type *sin_x_table_odd;
    custom_type *sin_y_table_odd;
    custom_type scale;
    custom_type A;


    char *input_file;
    FILE *file_handle;


    int k;                      // Number of data bits per block (4 for byte mode)
    int m;                      // Number of control bits
    int n;                      // Hamming code length (k + m)
    int total_bits;             // Extended code length (n + 1)

    uint8_t *hamming_buffer;    // Encoded bits
    int hamming_buffer_size;    // Size of buffer in bits
    int hamming_buffer_pos;     // Current position

    int file_exhausted;         // Flag indicating end of file

} Field_2_Context;



static const enum AVPixelFormat pix_fmts[] = {
        AV_PIX_FMT_YUV444P,
        AV_PIX_FMT_NONE
};

#define OFFSET_2(x) offsetof(Field_2_Context, x)
#define FLAGS_2 AV_OPT_FLAG_FILTERING_PARAM | AV_OPT_FLAG_VIDEO_PARAM

static const AVOption field_2_options[] = {
        { "data", "path to binary file to embed",
                OFFSET_2(input_file), AV_OPT_TYPE_STRING, {.str=NULL}, 0, 0, FLAGS_2 },
        { NULL }
};

AVFILTER_DEFINE_CLASS(field_2);



static int is_power_of_two(int x)
{
    return (x & (x - 1)) == 0;
}


static int calculate_control_bits(int data_bits)
{
    int r = 1;
    while ((1 << r) < data_bits + r + 1)
        ++r;
    return r;
}

static void init_hamming_params(Field_2_Context *s)
{
    s->k = 8;
    s->m = calculate_control_bits(s->k);
    s->n = s->k + s->m;
    s->total_bits = s->n + 1;
}


static void mat_mul(const custom_type *A, const custom_type *B, custom_type *C,
                    int a_rows, int a_cols, int b_cols) {
    for (int i = 0; i < a_rows; ++i) {
        for (int j = 0; j < b_cols; ++j) {
            custom_type sum = 0.0;
            for (int k = 0; k < a_cols; k++) {
                sum += A[i * a_cols + k] * B[k * b_cols + j];
            }
            C[i * b_cols + j] = sum;
        }
    }
}


static void build_dct_matrix_norm(custom_type* C, int n)
{
    custom_type scale = sqrt(2.0 / n);
    custom_type dc_scale = 1.0 / sqrt(n);

    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            if (i == 0)
            {
                C[i * n + j] = dc_scale;
            }
            else
            {
                custom_type angle = M_PI * i * (2 * j + 1) / (2 * n);
                C[i * n + j] = scale * cos(angle);
            }
        }
    }
}

static void transpose_matrix(const custom_type* src, custom_type* dst, int n)
{
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            dst[j * n + i] = src[i * n + j];
        }
    }
}

static void dct_2d_forward(Field_2_Context *s, custom_type* P, int n)
{
    custom_type* temp = s->dct_temp;
    mat_mul(P, s->CT, temp, n, n, n);
    mat_mul(s->C, temp, P, n, n, n);
}



static custom_type evaluate_texture_block(Field_2_Context *s,
                                          uint8_t* y_plane, int y_linesize,
                                          int start_x, int start_y)
{
    int block_size = s->block_size;
    custom_type *block = s->dct_block;

    int idx = 0;
    for (int y = 0; y < block_size; ++y)
    {
        int py = start_y + y;
        for (int x = 0; x < block_size; ++x)
        {
            int px = start_x + x;
            block[idx++] = (custom_type)y_plane[py * y_linesize + px] - 128.0;
        }
    }

    dct_2d_forward(s, block, block_size);

    custom_type abs_sum = 0.0;
    for (int i = 1; i < block_size * block_size; ++i)
    {
        abs_sum += fabs(block[i]);
    }

    return abs_sum;
}

static void build_mask(Field_2_Context *s, uint8_t* y_plane, int y_linesize)
{
    int block_size = s->block_size;
    int block_idx = 0;
    for (int by = 0; by < s->mask_height; ++by)
    {
        int start_y = by * block_size;
        for (int bx = 0; bx < s->mask_width; ++bx)
        {
            int start_x = bx * block_size;
            custom_type texture_score = evaluate_texture_block(s, y_plane, y_linesize,
                                                               start_x, start_y);
            s->mask[block_idx++] = (texture_score >= 255.0) ? 1 : 0;
        }
    }
}



static void add_sinusoid_to_block(uint8_t* plane, int linesize,
                                  int start_x, int start_y, int block_size,
                                  custom_type sin_x_table[], custom_type sin_y_table[],
                                  custom_type scale)
{
    for (int y = 0; y < block_size; ++y)
    {
        int py = start_y + y;
        custom_type sin_y = sin_y_table[py];

        for (int x = 0; x < block_size; ++x)
        {
            int px = start_x + x;
            custom_type sin_x = sin_x_table[px];
            custom_type total_shift = sin_x + sin_y;

            uint8_t original = plane[py * linesize + px];
            custom_type new_value = original * scale + total_shift;

            if (new_value > 255.0) new_value = 255.0;
            if (new_value < 0.0) new_value = 0.0;

            plane[py * linesize + px] = (uint8_t)new_value;
        }
    }
}


static void hamming_encode(Field_2_Context *s,
                           uint8_t byte,
                           uint8_t *encoded)
{
    for (int pos = 1; pos <= s->n; ++pos)
    {
        if (!is_power_of_two(pos))
        {
            encoded[pos - 1] = byte & 1;
            byte >>= 1;
        }
    }


    for (int i = 0; i < s->m; ++i)
    {
        int control_pos = (1 << i);
        int parity = 0;

        for (int pos = control_pos + 1; pos <= s->n; ++pos)
        {
            if (pos & control_pos)
                parity ^= encoded[pos - 1];
        }

        encoded[control_pos - 1] = parity;
    }


    int overall_parity = 0;
    for (int i = 0; i < s->n; ++i)
        overall_parity ^= encoded[i];

    encoded[s->n] = overall_parity;
}


static int open_data_file(Field_2_Context *s)
{
    if (!s->input_file)
        return -1;

    s->file_handle = fopen(s->input_file, "rb");
    if (!s->file_handle)
        return -1;

    fseek(s->file_handle, 0, SEEK_END);
    fseek(s->file_handle, 0, SEEK_SET);

    return 0;
}


static uint8_t load_next_byte(Field_2_Context *s)
{
    if (s->file_exhausted) return 0;

    uint8_t byte;
    if (fread(&byte, 1, 1, s->file_handle) != 1)
    {
        s->file_exhausted = 1;
        return 0;
    }
    return byte;
}


static uint8_t get_next_bit(Field_2_Context *s)
{
    if (s->hamming_buffer_pos >= s->hamming_buffer_size)
    {
        uint8_t encoded[13];
        for (int i = 0; i < 4; ++i)
        {
            uint8_t byte = load_next_byte(s);

            hamming_encode(s, byte, encoded);

            for (int j = 0; j < s->total_bits; ++j)
            {
                s->hamming_buffer[j * 4 + i] = encoded[j];
            }
        }

        s->hamming_buffer_pos = 0;
    }


    uint8_t bit = s->hamming_buffer[s->hamming_buffer_pos];
    ++s->hamming_buffer_pos;
    return bit;
}



static int config_props_output_2(AVFilterLink *outlink)
{
    AVFilterContext *ctx = outlink->src;
    Field_2_Context *s = ctx->priv;
    AVFilterLink *inlink = ctx->inputs[0];


    int width = inlink->w;
    int height = inlink->h;
    custom_type A = 5.0;

    s->h = height;
    s->w = width;
    s->frame_num = 0;
    s->block_size = 8;
    s->fps = (int)(1.0 / av_q2d(inlink->time_base) + 0.5);
    s->file_exhausted = 0;

    init_hamming_params(s);


    s->hamming_buffer_size = s->total_bits * 4;
    s->hamming_buffer_pos = s->total_bits * 4;
    s->hamming_buffer = av_malloc_array(s->hamming_buffer_size, sizeof(uint8_t));
    if (!s->hamming_buffer)
    {
        return AVERROR(ENOMEM);
    }


    int block_size = s->block_size;
    s->C = av_malloc_array(block_size * block_size, sizeof(custom_type));
    s->CT = av_malloc_array(block_size * block_size, sizeof(custom_type));
    s->dct_block = av_malloc_array(block_size * block_size, sizeof(custom_type));
    s->dct_temp = av_malloc_array(block_size * block_size, sizeof(custom_type));

    if (!s->C || !s->CT || !s->dct_block || !s->dct_temp)
    {
        return AVERROR(ENOMEM);
    }

    build_dct_matrix_norm(s->C, block_size);
    transpose_matrix(s->C, s->CT, block_size);


    s->scale = (255.0 - 2.0 * A) / 255.0;
    s->mask_width = width / block_size;
    s->mask_height = height / block_size;
    s->mask_size = s->mask_width * s->mask_height;
    s->mask = av_malloc_array(s->mask_size, sizeof(uint8_t));

    if (!s->mask)
    {
        return AVERROR(ENOMEM);
    }


    custom_type k = (custom_type)height / (custom_type)width;
    s->sin_x_table_even = av_malloc_array(width, sizeof(custom_type));
    s->sin_y_table_even = av_malloc_array(height, sizeof(custom_type));
    s->sin_x_table_odd = av_malloc_array(width, sizeof(custom_type));
    s->sin_y_table_odd = av_malloc_array(height, sizeof(custom_type));

    if (!s->sin_x_table_even || !s->sin_y_table_even ||
        !s->sin_x_table_odd || !s->sin_y_table_odd)
    {
        return AVERROR(ENOMEM);
    }

    for (int x = 0; x < width; ++x)
    {
        custom_type angle_x_even = 2.0 * M_PI * x * 50 / (width * k);
        custom_type angle_x_odd = 2.0 * M_PI * x * 66 / (width * k);
        s->sin_x_table_even[x] = A * cos(angle_x_even);
        s->sin_x_table_odd[x] = A * cos(angle_x_odd);
    }

    for (int y = 0; y < height; ++y)
    {
        custom_type angle_y_even = 2.0 * M_PI * y * 66 / height;
        custom_type angle_y_odd = 2.0 * M_PI * y * 50 / height;
        s->sin_y_table_even[y] = A * cos(angle_y_even);
        s->sin_y_table_odd[y] = A * cos(angle_y_odd);
    }

    open_data_file(s);

    outlink->w = width;
    outlink->h = height;
    outlink->time_base = inlink->time_base;

    return 0;
}


static int filter_frame_2(AVFilterLink *inlink, AVFrame *frame)
{
    Field_2_Context *s = inlink->dst->priv;
    AVFilterLink *outlink = inlink->dst->outputs[0];


    uint8_t current_bit = get_next_bit(s);
    av_log(s, AV_LOG_INFO, "Frame %d: embedding bit = %d\n", s->frame_num, current_bit);

    uint8_t* u_plane = frame->data[1];
    int u_linesize = frame->linesize[1];
    uint8_t* y_plane = frame->data[0];
    int y_linesize = frame->linesize[0];

    if (!u_plane || !y_plane)
    {
        s->frame_num++;
        return ff_filter_frame(outlink, frame);
    }

    int width = frame->width;
    int height = frame->height;
    int block_size = s->block_size;
    int blocks_x = width / block_size;
    int blocks_y = height / block_size;

    build_mask(s, y_plane, y_linesize);

    custom_type *sin_x_table, *sin_y_table;
    if (current_bit == 0)
    {
        sin_x_table = s->sin_x_table_even;
        sin_y_table = s->sin_y_table_even;
    }
    else
    {
        sin_x_table = s->sin_x_table_odd;
        sin_y_table = s->sin_y_table_odd;
    }

    int block_idx = 0;
    for (int by = 0; by < blocks_y; ++by)
    {
        int start_y = by * block_size;
        for (int bx = 0; bx < blocks_x; ++bx)
        {
            int start_x = bx * block_size;

            if (s->mask && block_idx < s->mask_size && s->mask[block_idx] == 1)
            {
                add_sinusoid_to_block(u_plane, u_linesize,
                                      start_x, start_y, block_size,
                                      sin_x_table, sin_y_table, s->scale);
            }
            block_idx++;
        }
    }

    s->frame_num++;
    return ff_filter_frame(outlink, frame);
}


static av_cold void uninit(AVFilterContext *ctx)
{
    Field_2_Context *s = ctx->priv;

    av_freep(&s->mask);
    av_freep(&s->C);
    av_freep(&s->CT);
    av_freep(&s->sin_x_table_even);
    av_freep(&s->sin_y_table_even);
    av_freep(&s->sin_x_table_odd);
    av_freep(&s->sin_y_table_odd);
    av_freep(&s->dct_block);
    av_freep(&s->dct_temp);
    av_freep(&s->hamming_buffer);

    if (s->file_handle)
    {
        fclose(s->file_handle);
        s->file_handle = NULL;
    }
}



static const AVFilterPad field_2_inputs[] = {
        {
                .name         = "default",
                .type         = AVMEDIA_TYPE_VIDEO,
                .filter_frame = filter_frame_2,
        },
};

static const AVFilterPad field_2_outputs[] = {
        {
                .name         = "default",
                .type         = AVMEDIA_TYPE_VIDEO,
                .config_props = config_props_output_2,
        },
};

const FFFilter ff_vf_field_2 = {
        .p.name        = "field_2",
        .p.description = NULL_IF_CONFIG_SMALL("Embed binary data into video using Extended Hamming code with zero padding."),
        .p.priv_class  = &field_2_class,
        .priv_size     = sizeof(Field_2_Context),
        .uninit        = uninit,
        FILTER_INPUTS(field_2_inputs),
        FILTER_OUTPUTS(field_2_outputs),
        FILTER_PIXFMTS_ARRAY(pix_fmts)
};
