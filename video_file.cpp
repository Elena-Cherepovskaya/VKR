//
// Created by lenac on 01.02.2026.
//

#include "video_file.hpp"

video_file::video_file()
    : input_format_ctx(nullptr), codec_ctx(nullptr),
      codec(nullptr), video_stream_index(-1),
      initialized(false),
      total_frames_read(0),
      decoding_finished(false), packet(nullptr), _frame(nullptr),
      flush_mode(false)
{
    _time_base.num = 0;
    _time_base.den = 1;
    frame_rate.num = 0;
    frame_rate.den = 1;
}

video_file::~video_file()
{
    cleanup();
}

void video_file::initialize_Fourier_arrays()
{
    int width = codec_ctx->width;
    int height = codec_ctx->height;

    custom_type k = (custom_type)height / (custom_type )width;

    for (int x = 0; x < width; x++)
    {
        custom_type angle_for_x = 2.0 * M_PI * (custom_type)x * 50.0 / (width * k);

        cos_array[0].first.push_back(cos(angle_for_x));
        sin_array[0].first.push_back(sin(angle_for_x));

        angle_for_x = 2.0 * M_PI * (custom_type)x * 66.0 / (width * k);

        cos_array[1].first.push_back(cos(angle_for_x));
        sin_array[1].first.push_back(sin(angle_for_x));
    }

    for (int y = 0; y < height; y++)
    {
        custom_type angle_for_y = 2.0 * M_PI * (custom_type)y * 50.0 / height;

        cos_array[1].second.push_back(cos(angle_for_y));
        sin_array[1].second.push_back(sin(angle_for_y));

        angle_for_y = 2.0 * M_PI * (custom_type)y * 66.0 / height;
        cos_array[0].second.push_back(cos(angle_for_y));
        sin_array[0].second.push_back(sin(angle_for_y));
    }
    int a = 0;
}


bool video_file::open(const std::string_view filename)
{
    if (avformat_open_input(&input_format_ctx, filename.data(), nullptr, nullptr) != 0)
    {
        std::cout << "Error opening video file: " << filename << std::endl;
        return false;
    }

    avformat_find_stream_info(input_format_ctx, nullptr);

    video_stream_index = -1;
    for (unsigned int i = 0; i < input_format_ctx->nb_streams; ++i)
    {
        if (input_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_index = i;
            break;
        }
    }


    AVStream* stream = input_format_ctx->streams[video_stream_index];
    _time_base = stream->time_base;
    frame_rate = stream->avg_frame_rate;

    // Настраиваем декодер
    AVCodecParameters* codec_params = stream->codecpar;
    codec = avcodec_find_decoder(codec_params->codec_id);

    codec_ctx = avcodec_alloc_context3(codec);

    avcodec_parameters_to_context(codec_ctx, codec_params);

    avcodec_open2(codec_ctx, codec, nullptr);

    packet = av_packet_alloc();
    _frame = av_frame_alloc();

    initialize_Fourier_arrays();

    initialized = true;
    print_video_info();
    return true;
}


void video_file::print_video_info() const
{
    if (!initialized) return;

    std::clog << "Resolution: " << codec_ctx->width << "x" << codec_ctx->height << std::endl;
    std::clog << "Frame rate: " << frame_rate.num << "/" << frame_rate.den
              << " (" << av_q2d(frame_rate) << " fps)" << std::endl;
    std::clog << "Duration: " << input_format_ctx->duration / 1000000.0 << " seconds" << std::endl;
    std::clog << "Codec: " << avcodec_get_name(codec_ctx->codec_id) << std::endl;
    std::clog << "Pixel format: " << av_get_pix_fmt_name((AVPixelFormat)codec_ctx->pix_fmt) << std::endl;
}


video_fragment video_file::get_next_one_second_fragment()
{
    video_fragment fragment;

    while (!decoding_finished)
    {
        if (!decode_next_frame())
            break;

        auto yuv_frame = std::make_shared<video_frame>(_frame, _time_base);
        fragment.add_frame(yuv_frame);

        if (fragment.get_duration () >= 1.0)
            return fragment;
    }
        
    decoding_finished = true;
    return fragment;
}


void video_file::cleanup()
{
    if (codec_ctx)
    {
        avcodec_free_context(&codec_ctx);
        codec_ctx = nullptr;
    }

    if (input_format_ctx)
    {
        avformat_close_input(&input_format_ctx);
        input_format_ctx = nullptr;
    }

    if (packet)
    {
        av_packet_free(&packet);
        packet = nullptr;
    }

    if (_frame)
    {
        av_frame_free(&_frame);
        _frame = nullptr;
    }

    initialized = false;
    total_frames_read = 0;
    decoding_finished = false;
    flush_mode = false;
}


bool video_file::decode_next_frame()
{
    while (true)
    {
        //Попытка получить кадр из буфера декодера
        int ret = avcodec_receive_frame(codec_ctx, _frame);

        if (ret == 0)
        {
            return true;
        }
        else if (ret == AVERROR(EAGAIN))
        {
            // Когда дочитываем кадры из буфера, отправить больше нечего
            if (flush_mode)
            {
                return false;
            }

            // Читаем новые пакеты из файла
            while (av_read_frame(input_format_ctx, packet) >= 0)
            {
                if (packet->stream_index == video_stream_index)
                {
                    ret = avcodec_send_packet(codec_ctx, packet);
                    av_packet_unref(packet);

                    if (ret < 0 && ret != AVERROR(EAGAIN))
                    {
                        std::cout << "Error sending packet to decoder" << std::endl;
                        return false;
                    }

                    ret = avcodec_receive_frame(codec_ctx, _frame);
                    if (ret == 0)
                    {
                        return true;
                    }
                    else if (ret == AVERROR(EAGAIN))
                    {
                        continue;
                    }
                    else
                    {
                        std::cout << "Error receiving frame" << std::endl;
                        return false;
                    }
                }
                else
                {
                    av_packet_unref(packet);
                }
            }

            // Достигнут конец файла. Переходим в режим flush и дочитываем буфер декодера
            flush_mode = true;
            // Отправка в буфер сигнала конца потока
            ret = avcodec_send_packet(codec_ctx, nullptr);
            if (ret < 0 && ret != AVERROR_EOF)
            {
                std::cout << "Error sending EOF packet" << std::endl;
                return false;
            }

            continue;
        }
        else if (ret == AVERROR_EOF)
        {
            decoding_finished = true;
            return false;
        }
        else
        {
            std::cout << "Error receiving frame" << std::endl;
            return false;
        }
    }
}
