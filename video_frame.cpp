//
// Created by lenac on 01.02.2026.
//

#include "video_frame.hpp"

video_frame::video_frame(const AVFrame* src_frame, AVRational time_base)
{
    _frame = av_frame_clone(src_frame);

    _width = src_frame->width;
    _height = src_frame->height;
    _pts = src_frame->pts;
    _format = yuv_format_from_av(src_frame->format);
    _is_linesize_positive = (src_frame->linesize[0] >= 0);

    _y_stride = std::abs(src_frame->linesize[0]);
    _u_stride = std::abs(src_frame->linesize[1]);
    _v_stride = std::abs(src_frame->linesize[2]);

    _timestamp_sec = pts_to_seconds(src_frame->pts, time_base);
}

video_frame::~video_frame()
{
    if (_frame)
    {
        av_frame_free(&_frame);
    }
    _frame = nullptr;
}



YUV_format video_frame::yuv_format_from_av(int av_pixel_format) const
{
    AVPixelFormat av_format = static_cast<AVPixelFormat>(av_pixel_format);
    switch (av_format)
    {
        case AV_PIX_FMT_YUV420P:
            return YUV_format::YUV420;
        case AV_PIX_FMT_YUV422P:
            return YUV_format::YUV422;
        case AV_PIX_FMT_YUV444P:
            return YUV_format::YUV444;
        default:
            return YUV_format::UNKNOWN;
    }
}


uint8_t video_frame::get_byte(int x, int y, int stride, int ind) const
{
    auto const correct_y = get_correct_y(y, stride);
    uint8_t* row_start = _frame->data[ind] + (correct_y * stride);
    return row_start[x];
}

bool video_frame::set_byte(int x, int y, uint8_t value, int stride, int ind)
{
    auto const correct_y = get_correct_y(y, stride);
    uint8_t* row_start = _frame->data[ind] + (correct_y * stride);
    row_start[x] = value;
    return true;
}

uint8_t video_frame::get_y_byte(int x, int y) const
{
    return get_byte(x, y, _y_stride, 0);
}

uint8_t video_frame::get_u_byte(int x, int y) const
{
    return get_byte(x, y, _u_stride, 1);
}

uint8_t video_frame::get_v_byte(int x, int y) const
{
    return get_byte(x, y, _v_stride, 2);
}

bool video_frame::set_y_byte(int x, int y, uint8_t value)
{
    return set_byte(x, y, value, _y_stride, 0);
}

bool video_frame::set_u_byte(int x, int y, uint8_t value)
{
    return set_byte(x, y, value, _u_stride, 1);
}

bool video_frame::set_v_byte(int x, int y, uint8_t value)
{
    return set_byte(x, y, value, _v_stride, 2);
}


// Ширину с выравниванием надо будет передавать
int video_frame::get_correct_y(int y, int width) const
{
    return _is_linesize_positive ? y : -(width - 1 - y);
}

std::vector<custom_type> video_frame::get_signal() const
{
    std::vector<custom_type> signal;

    int const u_linesize = _frame->linesize[1];
    for (int y = 0; y < _frame->height / 2; ++y)
    {
        for (int x = 0; x < _frame->width / 2; ++x)
        {
            signal.push_back(_frame->data[1][y * u_linesize + x]);
        }
    }
    return signal;
};





