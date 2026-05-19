//
// Created by lenac on 10.02.2026.
//


#pragma once

#include <vector>
#include <iostream>

#include "forward.hpp"

class Fourier_coefficients
{
private:
    int counter;
    custom_type sum_a;
    custom_type sum_b;
    custom_type phase;
    custom_type amplitude;

    custom_type cos_component;
    custom_type sin_component;

public:
    void begin();
    void push_param(custom_type angle, custom_type value) { push_param(std::sin(angle), std::cos(angle), value); }
    void push_param(custom_type sin, custom_type cos, custom_type value);
    void end();

    custom_type get_phase_new() const { return phase; }
    custom_type get_amplitude_new() const { return amplitude; }
};
