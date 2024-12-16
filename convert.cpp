/*
SC12 to FC32 file converter
Created by: Reintech Solutions Oy
Email: contact@reintech.fi

Usage: create a folder for .sc12 files, move converter executable there and run it
It will automatically create .fc32 versions of each .sc12 file in the same folder.
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>

namespace fs = std::filesystem;

const int BUFFER_SIZE = 250000;

void convertFile(const std::string& inputFilePath, const std::string& outputFilePath) {
    std::ifstream input(inputFilePath, std::ios::binary);
    if (!input) {
        std::cerr << "Failed to open input file: " << inputFilePath << std::endl;
        return;
    }

    std::ofstream output(outputFilePath, std::ios::binary);
    if (!output) {
        std::cerr << "Failed to open output file: " << outputFilePath << std::endl;
        return;
    }

    // Allocate buffers
    std::vector<uint8_t> buffer(3 * BUFFER_SIZE);
    std::vector<float> outBuffer(2 * BUFFER_SIZE);

    const float scaleFactor = 1.0f / 32767; // Scaling factor for normalization

    // Process the file chunk by chunk
    while (input) {
        input.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        std::streamsize bytesRead = input.gcount();

        // Number of complete samples read (3 bytes per 12 bit sample)
        size_t numSamples = bytesRead / 3;

        // Convert each sample
        size_t outIndex = 0;
        for (size_t i = 0; i < numSamples; ++i) {
            uint32_t a = buffer[i * 3 + 0];
            uint32_t b = buffer[i * 3 + 1];
            uint32_t c = buffer[i * 3 + 2];

            uint32_t packed = a | (b << 8) | (c << 16);

            // Extract and normalize the two 12-bit signed integers
            outBuffer[outIndex++] = static_cast<float>(int16_t((packed & 0xfff) << 4)) * scaleFactor;
            outBuffer[outIndex++] = static_cast<float>(int16_t(((packed >> 12) & 0xfff) << 4)) * scaleFactor;
        }

        output.write(reinterpret_cast<char*>(outBuffer.data()), outIndex * sizeof(float));

        if (output.fail()) {
            std::cerr << "Write failed for file: " << outputFilePath << std::endl;
            break;
        }
    }

    std::cout << "Converted: " << inputFilePath << " -> " << outputFilePath << std::endl;
}

int main() {
    try {
        // Get the current working directory
        fs::path currentDir = fs::current_path();

        // Iterate over all files in the current directory
        for (const auto& entry : fs::directory_iterator(currentDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".sc12") {
                std::string inputFilePath = entry.path().string();
                fs::path outputPath = entry.path();
                outputPath.replace_extension(".fc32");
                std::string outputFilePath = outputPath.string();

                // Convert the file
                convertFile(inputFilePath, outputFilePath);
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
