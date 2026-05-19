//
// Created by lenac on 10.02.2026.
//

#include "Fourier_coefficients.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

void Fourier_coefficients::begin()
{ 
    sum_a = 0;
    sum_b = 0;
    counter = 0;
}

void Fourier_coefficients::push_param(custom_type sin, custom_type cos, custom_type value)
{
    sum_a += value * cos;
    sum_b += value * sin;
    ++counter;
}

void Fourier_coefficients::end()
{
    auto sin_component = 2 * sum_a / counter;
    auto cos_component = 2 * sum_b / counter;
    phase = std::atan2(sin_component, cos_component);   
    amplitude = sqrt(sin_component * sin_component + cos_component * cos_component);
}
