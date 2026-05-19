#pragma once

#include "video_frame.hpp"

#include "forward.hpp"

class video_fragment
{
private:
    video_frame_array frames;
    video_fragment(const video_fragment &) = delete;
    video_fragment & operator=(const video_fragment &) = delete;

public:
    video_fragment() = default;
    ~video_fragment() = default;
    video_fragment(video_fragment && v)
    {
        frames.swap(v.frames);
    }

    video_fragment & operator=(video_fragment && v)
    {
        frames.swap(v.frames);
        return *this;
    }

    void add_frame(video_frame_ref);

    custom_type get_duration() const;

    bool is_empty() const { return frames.empty(); }

    const video_frame_array & get_frames() const & { return frames; }
};

