//
// Created by lenac on 01.02.2026.
//

#pragma once

#include <vector>
#include <iostream>


extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}

#include "Fourier_coefficients.hpp"
#include "forward.hpp"

enum class YUV_format
{
    YUV420,
    YUV422,
    YUV444,
    UNKNOWN
};


class video_frame
{
private:
    AVFrame* _frame;

    bool _is_linesize_positive;

    int _y_stride;
    int _u_stride;
    int _v_stride;

    int _width;
    int _height;
    custom_type _timestamp_sec;

    int64_t _pts;
    YUV_format _format;

public:
    video_frame(const AVFrame* src_frame, AVRational time_base);
    ~video_frame();

    video_frame(const video_frame&) = delete;
    video_frame& operator=(const video_frame&) = delete;

    static custom_type pts_to_seconds(int64_t pts, AVRational time_base)
    {
        return pts * av_q2d(time_base);
    }

private:
    YUV_format yuv_format_from_av(int av_pixel_format) const;

    uint8_t get_byte(int x, int y, int stride, int ind) const;

    bool set_byte(int x, int y, uint8_t value, int stride, int ind);

public:
    custom_type get_timestamp_sec() const {{ return _timestamp_sec; }; }

    uint8_t get_y_byte(int x, int y) const;
    uint8_t get_u_byte(int x, int y) const;
    uint8_t get_v_byte(int x, int y) const;

    bool set_y_byte(int x, int y, uint8_t value);
    bool set_u_byte(int x, int y, uint8_t value);
    bool set_v_byte(int x, int y, uint8_t value);

    // Ширину с выравниванием надо будет передавать
    int get_correct_y(int y, int width) const;

    std::vector<custom_type> get_signal() const;

    int get_width() const { return _width; };
    int get_height() const { return _height; };
    YUV_format get_format() const { return _format; };
    int64_t get_pts() const { return _pts; };
    AVFrame* get_frame() { return _frame; };
    uint8_t const * get_y_plane() const { return _frame->data[0];};
    int get_y_linesize() const { return _frame->linesize[0]; }


};


