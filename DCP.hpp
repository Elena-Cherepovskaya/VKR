#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <array>
#include <vector>

#include "forward.hpp"

template <int block_size>
class DCP
{
public:
    typedef std::array<custom_type, block_size * block_size> t_array;

    constexpr int get_block_size() const { return block_size; }
    
private:
    t_array C;
    t_array CT;
    t_array block;
    t_array temp;

    // Маска для блоков: true — блок текстурный, false — гладкий
    std::vector<bool> mask;
    int mask_width;
    int mask_height;

    void allocate_matrices()
    {
        build_dct_matrix_norm(C);
        transpose_matrix(C, CT);
    }

    static void mat_mul(const custom_type* A, const custom_type* B, custom_type* C,
                 int a_rows, int a_cols, int b_cols)
    {
        for (int i = 0; i < a_rows; i++)
        {
            for (int j = 0; j < b_cols; j++)
            {
                custom_type sum = 0.0;
                for (int k = 0; k < a_cols; k++)
                {
                    sum += A[i * a_cols + k] * B[k * b_cols + j];
                }
                C[i * b_cols + j] = sum;
            }
        }        
    }

    static void build_dct_matrix_norm(t_array & C)
    {
        custom_type scale = sqrt(2.0 / block_size);
        custom_type dc_scale = 1.0 / sqrt(block_size);

        for (int i = 0; i < block_size; i++)
        {
            for (int j = 0; j < block_size; j++)
            {
                if (i == 0)
                {
                    C[i * block_size + j] = dc_scale;
                }
                else
                {
                    custom_type angle = M_PI * i * (2 * j + 1) / (2 * block_size);
                    C[i * block_size + j] = scale * cos(angle);
                }
            }
        }
    }

    static void transpose_matrix(t_array const & src, t_array & dst)
    {
        for (int i = 0; i < block_size; i++)
        {
            for (int j = 0; j < block_size; j++)
            {
                dst[j * block_size + i] = src[i * block_size + j];
            }
        }        
    }

    void dct_2d_forward(t_array & P, t_array const & C, t_array const & CT, int n)
    {
        mat_mul(P.data(), CT.data(), temp.data(), n, n, n);
        mat_mul(C.data(), temp.data(), P.data(), n, n, n);
    }

public:
    DCP()
    {
        allocate_matrices();
        mask_width = 0;
        mask_height = 0;
    }
    ~DCP() = default;

    custom_type evaluate_texture_block(uint8_t const * y_plane,
        int y_linesize,
        int start_x, int start_y)
    {
        int idx = 0;
        for (int y = 0; y < block_size; y++)
        {
            int py = start_y + y;
            for (int x = 0; x < block_size; x++)
            {
                int px = start_x + x;
                block[idx++] = (custom_type)y_plane[py * y_linesize + px] - 128.0;
            }
        }

        dct_2d_forward(block, C, CT, block_size);

        custom_type abs_sum = 0.0;
        for (int i = 1; i < block_size * block_size; i++) {
            abs_sum += fabs(block[i]);
        }

        return abs_sum;
    }

    void build_mask(uint8_t const * y_plane,
        int y_linesize,
        int width, int height)
    {
        mask_width = width / block_size;
        mask_height = height / block_size;
        int mask_size = mask_width * mask_height;

        mask.resize(mask_size);

        int block_idx = 0;
        for (int by = 0; by < mask_height; by++)
        {
            int start_y = by * block_size;

            for (int bx = 0; bx < mask_width; bx++)
            {
                int start_x = bx * block_size;

                custom_type texture_score = evaluate_texture_block(y_plane, y_linesize,
                                                            start_x, start_y);

                mask[block_idx++] = (texture_score >= 255.0);
            }
        }
    }


    int get_mask_width() const { return mask_width; };

    int get_mask_height() const { return mask_height; };

    const std::vector<bool> & get_mask() const { return mask; }
};