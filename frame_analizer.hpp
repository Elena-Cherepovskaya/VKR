#pragma once

#include "DCP.hpp"
#include "sin_cos_data.hpp"
#include "Fourier_coefficients.hpp"

#include "forward.hpp"

#include <optional>

class frame_analizer
{
public:
    frame_analizer(video_file const & file);

    std::optional<bool> get_bit(video_frame_ref frame);

private:
    DCP<8>               dcp;
    sin_cos_data         sin_cos;
    Fourier_coefficients koeff_for[4];
    custom_type          v[4];
};