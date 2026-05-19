//
// Created by lenac on 02.04.2026.
//
#include "frame_analizer.hpp"

#include "video_frame.hpp"
#include "video_file.hpp"

frame_analizer::frame_analizer(video_file const & file)
    : sin_cos(file.get_width(), file.get_height())
{}

std::optional<bool> frame_analizer::get_bit(video_frame_ref frame)
{
    uint8_t const * y_plane = frame->get_y_plane();
    auto const width = frame->get_width();
    auto const height = frame->get_height();
    auto const y_linesize = frame->get_y_linesize();

    dcp.build_mask(y_plane, y_linesize, width, height);

    const std::vector<bool> & mask = dcp.get_mask();

    bool const direction[] = {true, false, true, false};

    auto const u_width = width / 2;
    auto const u_height = height / 2;

    auto const & signal = frame->get_signal();

    auto block_size = dcp.get_block_size();
    auto const blocks_per_row = width / block_size;

    for(auto j = 0; j < 4; ++j)
        koeff_for[j].begin();


    for(auto y = 0; y < u_height; ++y)
    {
        auto const original_y = y * 2;
        auto const block_y = original_y / block_size;
        auto const y_offset = y * u_width;


        for(auto x = 0; x < u_width; ++x)
        {
            auto const original_x = x * 2;
            auto const block_x = original_x / block_size;
            auto const block_idx = block_y * blocks_per_row + block_x;

            if (block_idx < mask.size() && mask[block_idx])
            {
                custom_type const s = signal[y_offset + x];

                for(auto j = 0; j < 4; ++j)
                {
                    auto const idx = direction[j] ? original_x : original_y;
                    koeff_for[j].push_param(
                            sin_cos.get_sin_table(j)[idx],
                            sin_cos.get_cos_table(j)[idx], s);
                }
            }
        }
    }


    for(auto j = 0; j < 4; ++j)
    {
        koeff_for[j].end();
        v[j] = koeff_for[j].get_amplitude_new();
    }

    std::optional<bool> value_in_frame;
    if (v[0] > v[2] && v[1] > v[3])
        value_in_frame = false;

    if (v[2] > v[0] && v[3] > v[1])
        value_in_frame = true;

    return value_in_frame;
}