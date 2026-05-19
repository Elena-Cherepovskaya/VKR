//
// Created by lenac on 04.04.2026.
//

#include <memory>
#include <vector>

typedef float custom_type;

class video_file;

class video_frame;
typedef std::shared_ptr<video_frame> video_frame_ptr;
typedef video_frame_ptr const & video_frame_ref;

typedef std::vector<video_frame_ptr> video_frame_array;