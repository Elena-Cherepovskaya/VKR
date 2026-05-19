#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}

#include <string>
#include <iostream>
#include <memory>
#include <cstring>
#include <cmath>

#include "video_fragment.hpp"
#include "video_frame.hpp"
#include "forward.hpp"


class video_file
{
private:
    AVFormatContext* input_format_ctx;
    AVCodecContext* codec_ctx;
    const AVCodec* codec;
    int video_stream_index;
    AVRational _time_base;
    AVRational frame_rate;

    bool initialized;


    bool decoding_finished;
    AVPacket* packet;
    AVFrame* _frame;
    int total_frames_read;
    int fragment_counter;
    bool flush_mode;

    int freq_base = 100;

    std::pair<std::vector<custom_type>, std::vector<custom_type>> sin_array[2];
    std::pair<std::vector<custom_type>, std::vector<custom_type>> cos_array[2];

public:
    video_file();
    ~video_file();

    bool open(const std::string_view filename);
    
    int get_width() const { return codec_ctx->width; }
    int get_height() const { return codec_ctx->height; }


    void print_video_info() const;

    video_fragment get_next_one_second_fragment();

private:
    void initialize_Fourier_arrays();
    bool decode_next_frame();
    void cleanup();    
};
