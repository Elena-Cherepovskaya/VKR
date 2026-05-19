#include "sin_cos_data.hpp"

#include "forward.hpp"

#define _USE_MATH_DEFINES
#include <cstdlib>
#include <math.h>
#include <cmath>

sin_cos_data::sin_cos_data(int width, int height)
{
    custom_type const k = (custom_type)height / (custom_type)width;

    initialize(cos_for_zero_x, sin_for_zero_x, width, 2.0 * M_PI * 50.0 / (width * k));
    initialize(cos_for_one_x, sin_for_one_x, width, 2.0 * M_PI * 66.0 / (width * k));

    initialize(cos_for_one_y, sin_for_one_y, height, 2.0 * M_PI * 50.0 / height);
    initialize(cos_for_zero_y, sin_for_zero_y, height, 2.0 * M_PI * 66.0 / height);
}

std::vector<custom_type> const & sin_cos_data::get_sin_table(int index)
{
    std::vector<custom_type> * table[4] = {
        & sin_for_zero_x,
        & sin_for_zero_y,
        & sin_for_one_x,
        & sin_for_one_y
    };
    return * table[index];
}

std::vector<custom_type> const & sin_cos_data::get_cos_table(int index)
{
    std::vector<custom_type> * table[4] = {
        & cos_for_zero_x,
        & cos_for_zero_y,
        & cos_for_one_x,
        & cos_for_one_y
    };
    return * table[index];
}

void sin_cos_data::initialize(
    std::vector<custom_type> & in_cos,
    std::vector<custom_type> & in_sin,
    int lenght, custom_type step)
{
    custom_type angle = 0;
    for (int x = 0; x < lenght; ++x, angle += step)
    {
        in_cos.push_back(std::cos(angle));
        in_sin.push_back(std::sin(angle));
    }
}
