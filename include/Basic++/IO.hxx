#pragma once
#include <cstdio>
#include <cstddef>
#include "Basic++/Expected.hxx"

/* Why would you use this header file instead of just writing traditional manual file I/O?
    - Well, I'm trying to simplify file reading and writing, turn them more into verbs instead
      of receipes, which it what it feels like using the C file API.
*/
namespace Basic::IO
{
    using namespace Basic::Expectations;

    enum class FileWriteOptions : std::uint8_t { WriteReplace, WriteAppend };
    enum class FileReadOptions : std::uint8_t { ReadOSNative, ReadBinary };

    static Err Write_File(
        const std::string_view& file_path,
        const std::string& to_write,
        FileWriteOptions options = FileWriteOptions::WriteReplace,
        bool binary = true)
    {
        if (file_path.empty())
            return { false,"supplied invalid `file_path`" };

        FILE* file;

        switch (options)
        {
        case FileWriteOptions::WriteReplace:
            file = std::fopen(file_path.data(), binary ? "wb" : "w");
            break;
        case FileWriteOptions::WriteAppend:
            file = std::fopen(file_path.data(), binary ? "ab" : "a");
            break;
        default:
            return { false, "Unrecognized enum `options` value" };
        }

        if (!file) // TODO: I can't really get exact errors, I should probably use the win32 API.
            return "file system error, file couldn't be opened";

        std::size_t bytes_written = std::fwrite(to_write.data(), sizeof(char), to_write.length(), file);

        if (bytes_written < to_write.length() or bytes_written < 0)
            return { false, "couldn't write all bytes from `to_write` to file" };

        std::fclose(file);

        return true;
    }

    static Err Write_File(
        const std::string_view& file_path,
        const char* to_write,
        std::size_t to_write_size,
        FileWriteOptions options = FileWriteOptions::WriteReplace,
        bool binary = true)
    {
        if (file_path.empty())
            return { false,"supplied invalid `file_path`" };

        FILE* file;

        switch (options)
        {
        case FileWriteOptions::WriteReplace:
            file = std::fopen(file_path.data(), binary ? "wb" : "w");
            break;
        case FileWriteOptions::WriteAppend:
            file = std::fopen(file_path.data(), binary ? "ab" : "a");
            break;
        default:
            return { false, "Unrecognized enum `options` value" };
        }

        if (!file) // TODO: I can't really get exact errors, I should probably use the win32 API.
            return "file system error, file couldn't be opened";

        std::size_t bytes_written = std::fwrite(to_write, sizeof(char), to_write_size, file);

        if (bytes_written < to_write_size or bytes_written < 0)
            return { false, "couldn't write all bytes from `to_write` to file" };

        std::fclose(file);

        return true;
    }

    static Result<std::string> Read_File(
        const std::string_view& file_path,
        FileReadOptions options = FileReadOptions::ReadBinary)
    {
        Result<std::string> contents;

        FILE* file;

        switch (options)
        {
        case FileReadOptions::ReadOSNative:
            file = std::fopen(file_path.data(), "r");
            break;
        case FileReadOptions::ReadBinary:
            file = std::fopen(file_path.data(), "rb");
            break;
        default:
            return { {}, "Unrecognized enum `options` value" };
        }

        if (!file)
            return { {}, "file system error, file couldn't be opened" };

        std::fseek(file, 0, SEEK_END);

        std::size_t file_length = std::ftell(file);

        std::rewind(file);

        contents->resize(file_length);

        std::fread(contents->data(), sizeof(char), file_length, file);

        std::fclose(file);

        return contents;
    }
}
