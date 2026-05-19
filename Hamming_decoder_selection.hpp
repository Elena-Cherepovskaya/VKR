//
// Created by lenac on 08.05.2026.
//

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <optional>
#include <iostream>


class Hamming_decoder_selection
{
private:
    std::vector<bool> received_word;
    std::vector<int> unknown_positions;
    int k;
    int m;
    int n;
    int total_bits;

    bool is_power_of_two(int x);

    int calculate_control_bits_count(int data_length);

    int calculate_k_from_length(int total_bits);

    bool is_valid_code(const std::vector<bool>& full_word);


    std::vector<std::vector<bool>> try_combinations(
        std::vector<bool> & word,
        std::vector<int> const & unknown_positions,
        int index);

    uint8_t extract_data(const std::vector<bool> & bit_array);

    void init();
    void reset();

public:

    void push_bit(std::optional<bool> const & bit);

    std::optional<uint8_t> pop_byte();
};