#ifndef TDF_WRITER_FILE_COLLECTOR_HPP
#define TDF_WRITER_FILE_COLLECTOR_HPP

#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <stdexcept>


#include "dispatcher.hpp"
#include "simple_buffer.hpp"


class FileCollector : public Reducer<SimpleBuffer<char>>
{
    FILE* file = nullptr;

public:
    FileCollector(std::string filename)
    {
        file = std::fopen(filename.c_str(), "wb");
        if(!file) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }

    FileCollector(const FileCollector&) = delete;
    FileCollector& operator=(const FileCollector&) = delete;
    FileCollector(FileCollector&& other) noexcept : file(other.file)
    {
        other.file = nullptr;
    }
    FileCollector& operator=(FileCollector&&) = delete;

    void reduce(const SimpleBuffer<char>& input) override
    {
        std::fwrite(input.data(), 1, input.size(), file);
    }

    ~FileCollector()
    {
        if(file) {
            std::fclose(file);
        }
    }
};


#endif // TDF_WRITER_FILE_COLLECTOR_HPP