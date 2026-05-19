cmake_minimum_required(VERSION 3.20)
project(video_processor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(FFMPEG_DIR "C:/ffmpeg")


include_directories("${FFMPEG_DIR}/include")
link_directories("${FFMPEG_DIR}/lib")


add_executable(video_processor
        main.cpp
        video_fragment.cpp
        video_file.cpp
        video_frame.cpp
        Fourier_coefficients.cpp
        sin_cos_data.cpp
        frame_analizer.cpp
        Hamming_decoder_selection.cpp
)

target_link_libraries(video_processor
        "avcodec"
        "avformat"
        "avutil"
        "swresample"
        "swscale"
        "avfilter"
)
