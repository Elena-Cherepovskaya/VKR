#include "video_file.hpp"
#include "DCP.hpp"
#include "sin_cos_data.hpp"
#include "frame_analizer.hpp"
#include "forward.hpp"
#include "Hamming_decoder_selection.hpp"

#include <iostream>
#include <optional>
#include <vector>
#include <cctype>
#include <bitset>
#include <fstream>
#include <cstring>
#include <cmath>




int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "  input_video  - path to video file with embedded data" << std::endl;
        std::cout << "  output_file  - path to output text file" << std::endl;
        return 1;
    }

    std::string_view const input_video = argv[1];
    std::string_view const output_file = argv[2];

    video_file file;

    if (!file.open(input_video))
    {
        std::cout << "Open video failed" << std::endl;
        return 1;
    }
    std::cout << std::endl;

    frame_analizer analizer(file);

    std::vector<uint8_t> bytes;

    Hamming_decoder_selection decoder[4];

    int i = 0;
    video_fragment fragment(file.get_next_one_second_fragment());
    for(int n = 0; !fragment.is_empty(); ++n, (fragment = file.get_next_one_second_fragment()))
    {
        std::clog << "Fragment " << n << std::endl;

        for (auto && frame : fragment.get_frames())
        {           
            auto const & value_in_frame = analizer.get_bit(frame);

#ifndef NDEBUG
            std::clog << "Frame " << i << " => ";
            
            uint8_t bit = 0;
            if (value_in_frame.has_value())
            {
                bit = value_in_frame.value() ? 1 : 0;
                std::clog << (int)bit << std::endl;
            }
            else
            {
                std::clog << '?' << std::endl;
            }
#endif

            auto & d = decoder[i % 4];

            d.push_bit(value_in_frame);
            auto const byte = d.pop_byte();


            if (byte.has_value())
            {
                bytes.push_back(byte.value());
                std::clog << "Byte: " << byte.value() << std::endl;
            }

            ++i;
        }
    }

    {
        std::ofstream out_file(output_file.data(), std::ios::binary);
        if (out_file.is_open())
        {
            out_file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
            out_file.close();
        }
        else
        {
            std::cerr << "Failed to create output file: " << output_file << std::endl;
        }
    }

    std::clog << std::endl << "Decoded Bytes" << std::endl;
    for (auto && byte : bytes)
        std::clog << std::bitset<8>(byte) << " - "
                  << ((byte >= 32 && byte <= 126) ? (char)byte : '?') << std::endl;

    return 0;
}