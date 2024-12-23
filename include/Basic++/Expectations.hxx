#pragma once

#include <source_location>
#include <filesystem>

#include "Basic++/Printing.hxx"
#include "Basic++/Message.hxx"

namespace Basic
{
    inline namespace ResultType
    {
        template<typename T>
        struct Result;
    }
}

namespace Basic
{
    inline namespace Expectations
    {
        static bool Expect(
            bool condition,
            const Message&& msg,
            const std::source_location& source_location = std::source_location::current());

        template<typename T>
        bool Expect(
            Result<T> expected,
            const std::source_location& source_location = std::source_location::current());

    }
}

// TODO: This should be here, but it's easy to remove once it REALLY doesn't need to be here... so.
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#include <debugapi.h> // For DebugBreak()

// Later on, we should use a modified version of https://github.com/scottt/debugbreak.
#define BASIC_DEBUG_BREAK() DebugBreak();

#ifndef NO_EXPECTATIONS
#define EXPECT(_cond_, _msg_, ...)                                                                         \
        if (!Basic::Expectations::Expect((_cond_), {_msg_, __VA_ARGS__}, std::source_location::current())) \
		{		                                                                                           \
            std::fflush(stdout);																           \
			BASIC_DEBUG_BREAK();                                                                           \
            std::exit(EXIT_FAILURE);									                                   \
		} 																					               

// E (Result) Expect
#define EEXPECT(_expected_)                                                                                           \
[&]() {                                                                                                               \
    auto expected = _expected_;                                                                                       \
    auto expected_value = *expected;                                                                                  \
    if (!Basic::Expectations::Expect<typename decltype(_expected_)::Type>(expected, std::source_location::current())) \
    {                                                                                                                 \
        std::fflush(stdout);																                          \
        BASIC_DEBUG_BREAK();                                                                                          \
        std::exit(EXIT_FAILURE);                                                                                      \
    }                                                                                                                 \
    return expected_value;\
    }()

#else

#define EXPECT(_cond_, _msg_, ...)
#define EEXPECT(_expected_) *_expected_

#endif

// this header relies on this file
#include "Basic++/Result.hxx"

namespace Basic
{
    bool Expectations::Expect(
        bool condition,
        // FIXME: this is bad, every single time we call this function, even if it "succeeds"
        //        the Message class performs format on the string given. We don't need it 
        //        to do that if the function succeeds, right? Because the msg is only for failures.
        //        well, most of the time.
        const Message&& msg,
        const std::source_location& source_location)
    {
        if (!condition) [[unlikely]]
        {
            Basic::Print("expectation not satisfied : '{}'", msg.string.data());

            const auto file_name =
                std::filesystem::path(source_location.file_name()).filename().string();

            const char* function_name = source_location.function_name();

            Basic::Printing::Println(" ~ file:'{}', function:'{}', line:'{}:{}'",
                file_name.data(), function_name, source_location.line(), source_location.column());
        }

        return condition;
    }

    template<typename T>
    bool Expectations::Expect(
        Result<T> expected,
        const std::source_location& source_location)
    {
        return Expect(expected == true, Message{ expected.status() }, source_location);
    }
}