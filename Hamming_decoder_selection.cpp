//
// Created by lenac on 11.05.2026.
//

#include "Hamming_decoder_selection.hpp"

#include <algorithm>

bool Hamming_decoder_selection::is_power_of_two(int x)
{
    return (x & (x - 1)) == 0;
}

int Hamming_decoder_selection::calculate_control_bits_count(int data_length)
{
    int r = 1;
    while ((1 << r) < data_length + r + 1)
        ++r;
    return r;
}

int Hamming_decoder_selection::calculate_k_from_length(int total_bits)
{
    for (int test_k = 1; test_k <= total_bits; ++test_k)
    {
        int test_m = calculate_control_bits_count(test_k);
        int test_n = test_k + test_m;
        int test_total = test_n + 1;

        if (test_total == total_bits)
        {
            return test_k;
        }
    }
    return 0;
}

bool Hamming_decoder_selection::is_valid_code(const std::vector<bool> & full_word)
{
    int n_hamming = full_word.size() - 1;
    std::vector<uint8_t> hamming_part(full_word.begin(), full_word.end() - 1);


    int overall_parity = 0;
    for (uint8_t bit : full_word)
        overall_parity ^= bit;
    if (overall_parity != 0)
        return false;


    for (int i = 0; i < m; ++i)
    {
        int control_pos = (1 << i);
        int parity = 0;

        for (int pos = control_pos + 1; pos <= n_hamming; ++pos)
        {
            if (pos & control_pos)
            {
                parity ^= hamming_part[pos - 1];
            }
        }

        if (parity != hamming_part[control_pos - 1])
            return false;
    }

    return true;
}



std::vector<std::vector<bool>> Hamming_decoder_selection::try_combinations(
    std::vector<bool> & word,
    std::vector<int> const & unknown_positions,
    int index)
{
    std::vector<std::vector<bool>> valid_words;
    
    if (index == unknown_positions.size())
    {
        if (is_valid_code(word))
            valid_words.push_back(word);
            
        return valid_words;
    }

    int const pos = unknown_positions[index];

    for(int i = 0; i < 2; ++i)
    {
        word[pos] = i == 1;
        auto res = try_combinations(word, unknown_positions, index + 1);
        valid_words.insert(valid_words.end(), res.begin(), res.end());
    }

    return valid_words;
}



uint8_t Hamming_decoder_selection::extract_data(const std::vector<bool> & bit_array)
{
    std::vector<bool> hamming_part(bit_array.begin(), bit_array.end() - 1);

    uint8_t result_byte = 0;
    int bit_position = 0;

    for (int pos = 1; pos <= n; ++pos)
    {
        if (!is_power_of_two(pos))
        {
            result_byte |= (hamming_part[pos - 1] << bit_position);
            ++bit_position;
        }
    }

    return result_byte;
}


void Hamming_decoder_selection::push_bit(std::optional<bool> const & bit)
{
    if (bit.has_value())
        received_word.push_back(bit.value());
    else
    {
        unknown_positions.push_back(received_word.size());
        received_word.push_back(false);
    }
}

void Hamming_decoder_selection::init()
{
    total_bits = received_word.size();
    k = calculate_k_from_length(total_bits);
    m = calculate_control_bits_count(k);
    n = k + m;
}

void Hamming_decoder_selection::reset()
{
    unknown_positions.clear();
}

std::optional<uint8_t> Hamming_decoder_selection::pop_byte()
{
    if (received_word.size() != 13)
        return std::nullopt;

    init();

    std::optional<uint8_t> res;


    if (unknown_positions.empty())
    {
        reset();

        if (is_valid_code(received_word))
        {
            std::clog << "The word is correct, no unknown bits were detected." << std::endl;
            res = extract_data(received_word);
            received_word.clear();
            return res;
        }

        std::clog << "There are incorrect bits in the word." << std::endl;
        res = extract_data(received_word);
        received_word.clear();
        return res;
    }

    auto const & valid_words = try_combinations(received_word, unknown_positions, 0);

    if (valid_words.empty())
    {
        reset();
        std::clog << "No valid combinations found" << std::endl;
        res = extract_data(received_word);
        received_word.clear();
        return res;
    }

    if (valid_words.size() == 1)
    {
        std::clog << "Unique solution found" << std::endl;
        res = extract_data(valid_words[0]);
    }
    else
    {
        std::clog << "Multiple solutions found (" + std::to_string(valid_words.size()) + " possibilities)" << std::endl;
        res = extract_data(valid_words[0]);
    }

    reset();
    received_word.clear();
    return res;
}
