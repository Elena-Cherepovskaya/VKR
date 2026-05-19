#pragma once

#include <vector>

#include "forward.hpp"

class sin_cos_data
{    
public:
    sin_cos_data(int width, int height);

    std::vector<custom_type> const & get_sin_for_zero_x() const { return sin_for_zero_x; }
    std::vector<custom_type> const & get_sin_for_one_x() const { return sin_for_one_x; }

    std::vector<custom_type> const & get_cos_for_zero_x() const { return cos_for_zero_x; }
    std::vector<custom_type> const & get_cos_for_one_x() const { return cos_for_one_x; }

    std::vector<custom_type> const & get_sin_for_zero_y() const { return sin_for_zero_y; }
    std::vector<custom_type> const & get_sin_for_one_y() const { return sin_for_one_y; }
    
    std::vector<custom_type> const & get_cos_for_zero_y() const { return cos_for_zero_y; }
    std::vector<custom_type> const & get_cos_for_one_y() const { return cos_for_one_y; }

    std::vector<custom_type> const & get_sin_table(int index);
    std::vector<custom_type> const & get_cos_table(int index);

private:
    static void initialize(
        std::vector<custom_type> & cos,
        std::vector<custom_type> & sin,
        int len, custom_type step);

    std::vector<custom_type> sin_for_zero_x;
    std::vector<custom_type> sin_for_one_x;
    std::vector<custom_type> cos_for_zero_x;
    std::vector<custom_type> cos_for_one_x;

    std::vector<custom_type> sin_for_zero_y;
    std::vector<custom_type> sin_for_one_y;
    std::vector<custom_type> cos_for_zero_y;
    std::vector<custom_type> cos_for_one_y;
};