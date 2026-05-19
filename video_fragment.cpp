//
// Created by lenac on 01.02.2026.
//

#include "video_fragment.hpp"
#include "forward.hpp"

custom_type video_fragment::get_duration() const
{
    if (frames.size() < 2)
        return 0;
    else
    {
        auto const duration = (*frames.rbegin())->get_timestamp_sec() - (*frames.begin())->get_timestamp_sec();
        return duration + duration / (frames .size() - 1);
    }
}

void video_fragment::add_frame(video_frame_ref frame)
{
    frames.push_back(frame);
}
