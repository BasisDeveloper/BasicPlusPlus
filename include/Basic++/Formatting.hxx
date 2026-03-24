#pragma once

// TODO: make this work without breaking everything, thanks.
#ifndef BASIC_FORMATTING_NO_STD_STRING
#include <string>
#endif

#include <cstring>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <typeinfo>
#include <cmath>
#include <charconv>

namespace Basic::Formatting
{
    #pragma region All Of This... Just to remove const from a type...
    template <typename T>
    struct erase_const_impl { typedef T type; };

    template <typename T>
    struct erase_const_impl<const T> { typedef T type; };

    template <typename T>
    struct erase_const_impl<T*> { typedef typename erase_const_impl<T>::type* type; };

    template <typename T>
    struct erase_const_impl<T* const> { typedef typename erase_const_impl<T>::type* const type; };

    template <typename T>
    struct erase_const_impl<T&> { typedef typename erase_const_impl<T>::type& type; };

    template <typename T>
    struct erase_const_impl<T&&> { typedef typename erase_const_impl<T>::type&& type; };

    template <typename T> using erase_const = typename erase_const_impl<T>::type;

    #pragma endregion

    template <typename T>
    concept Formattable = std::integral<T>
        or std::floating_point<T>
        or std::is_same<erase_const<T>, char*>::value
        or std::is_same<erase_const<T>, char[]>::value
        or std::is_same<erase_const<T>, std::string>::value
        or std::is_same<erase_const<T>, std::string_view>::value
        or std::is_pointer<T>::value;

    template<typename T>
    auto _Get_Address_If_Needed(const auto& v) -> auto* const
    {
        if constexpr (std::is_pointer<T>() == true)
            return v; // if it's already a pointer type, return the pointer type.
        else
            return &v; // if it's not, return the address of the storage_type.
    };

    struct FormattableTypeInformation
    {
        std::size_t type_hash;
        /* What I mean by a "raw pointer" is a pointer that
           isn't a char*, const char*, or char[], it's pointer
           that doesn't point towards a string. */
        bool raw_pointer;
    };

    /* An overload that doesn't take any arguments, so user template code is easier,
       and it's possible to just to ` Format("Hello") `; */
    static std::string Format(const char* msg)
    {
        return std::string(msg);
    }

    template <Formattable... formatables_t>
    static std::string Format(const char* msg, formatables_t... args)
    {
        auto character_is_number = [](const char& c) { return c >= '0' and c <= '9'; };

        const std::size_t msg_length = strlen(msg) + 1;
        const char* msg_end = msg + msg_length;

        const FormattableTypeInformation type_informations[sizeof...(args)] =
        {
            (FormattableTypeInformation
            {
                .type_hash = typeid(decltype(std::remove_extent_t<erase_const<decltype(args)>>())).hash_code(),

                .raw_pointer = std::is_pointer<decltype(args)>()
                               and !(std::is_same_v<decltype(args), const char*>
                                     or std::is_same_v<decltype(args), char*>)
            })...
        };

        const void* const ptrs_to_args_data[sizeof...(args)]{ _Get_Address_If_Needed<decltype(args)>(args)... };

        /* When you use the {} syntax for substitution,
           for each time it's found, we print the next element in the args.
           Format("{} {}", "Hello", "World") > "Hello World" */
        std::size_t ptrs_handled_count = 0;

        std::string out_formatted = {};

        constexpr const char failure_string[] = "`FAIL`";

        auto Get_Format_Substitution = [&](std::size_t index) -> std::string
            {
                auto Number_To_String = [&index, &ptrs_to_args_data]<typename T>(T number) -> std::string
                {
                    char num_buffer[256] = {};
                    auto [_, ec] = std::to_chars(num_buffer, num_buffer + sizeof(num_buffer), *(T*)ptrs_to_args_data[index]);
                    assert(ec == std::errc() && "failure parsing number.");
                    return std::string(num_buffer);
                };

                if (type_informations[index].raw_pointer == true)
                {
                    char num_buffer[256] = {};
                    auto [_, ec] = std::to_chars(num_buffer, num_buffer + sizeof(num_buffer), (std::int64_t)ptrs_to_args_data[index], 16); // 16 because we want hexadecimal.
                    assert(ec == std::errc() && "failure parsing number.");
                    return std::string(num_buffer);
                }

                if (type_informations[index].type_hash == typeid(char*).hash_code())
                {
                    return std::string((char*)ptrs_to_args_data[index]);
                }

                if (type_informations[index].type_hash == typeid(char).hash_code())
                {
                    std::string out_character;
                    out_character += *(char*)ptrs_to_args_data[index];
                    return out_character;
                }
                else if (type_informations[index].type_hash == typeid(unsigned char).hash_code())
                {
                    std::string out_character;
                    out_character += *(unsigned char*)ptrs_to_args_data[index];
                    return out_character;
                }
                else if (type_informations[index].type_hash == typeid(signed char).hash_code())
                {
                    std::string out_character;
                    out_character += *(char*)ptrs_to_args_data[index];
                    return out_character;
                }

                if (type_informations[index].type_hash == typeid(int).hash_code())
                {
                    return Number_To_String(*(int*)ptrs_to_args_data[index]);
                }
                else if (type_informations[index].type_hash == typeid(unsigned int).hash_code())
                {
                    return Number_To_String(*(unsigned int*)ptrs_to_args_data[index]);
                }

                if (type_informations[index].type_hash == typeid(short).hash_code())
                {
                    return Number_To_String(*(short*)ptrs_to_args_data[index]);
                }
                else if (type_informations[index].type_hash == typeid(unsigned short).hash_code())
                {
                    return Number_To_String(*(unsigned short*)ptrs_to_args_data[index]);
                }

                if (type_informations[index].type_hash == typeid(long).hash_code())
                {
                    return Number_To_String(*(long*)ptrs_to_args_data[index]);
                }
                else if (type_informations[index].type_hash == typeid(unsigned long).hash_code())
                {
                    return Number_To_String(*(unsigned long*)ptrs_to_args_data[index]);
                }

                if (type_informations[index].type_hash == typeid(long long).hash_code())
                {
                    return Number_To_String(*(long long*)ptrs_to_args_data[index]);
                }
                else if (type_informations[index].type_hash == typeid(unsigned long long).hash_code())
                {
                    return Number_To_String(*(unsigned long long*)ptrs_to_args_data[index]);
                }

                if (type_informations[index].type_hash == typeid(float).hash_code())
                {
                    return Number_To_String(*(float*)ptrs_to_args_data[index]);
                }

                if (type_informations[index].type_hash == typeid(double).hash_code())
                {
                    return Number_To_String(*(double*)ptrs_to_args_data[index]);
                }

                if (type_informations[index].type_hash == typeid(long double).hash_code())
                {
                    return Number_To_String(*(long double*)ptrs_to_args_data[index]);
                }

                if (type_informations[index].type_hash == typeid(bool).hash_code())
                {
                    std::string out_string;
                    bool boolean = *(bool*)ptrs_to_args_data[index];
                    out_string = boolean == true ? "true" : "false";
                    return out_string;
                }

                if (type_informations[index].type_hash == typeid(std::string).hash_code())
                {
                    std::string* ptr_to_std_string = (std::string*)ptrs_to_args_data[index];
                    return (ptr_to_std_string->data());
                }

                if (type_informations[index].type_hash == typeid(std::string_view).hash_code())
                {
                    std::string_view* ptr_to_std_string_view = (std::string_view*)ptrs_to_args_data[index];
                    return std::string(*ptr_to_std_string_view);
                }

                return std::string(failure_string);
            };

        const char* head_ptr = msg;

        while (head_ptr != msg_end)
        {
            // TODO: So, I need to add some error checking for runtime and compile time.

            const char* look_ahead_ptr = head_ptr;

            if (*look_ahead_ptr == '{')
            {
                look_ahead_ptr += 1;

                if (*(look_ahead_ptr) == '}')
                {
                    std::string substitution = Get_Format_Substitution(ptrs_handled_count);

                    assert(substitution != failure_string
                        && "Received a data type `Format` doesn't understand, or there weren't enough substitutions ('{}') for the amount of arguments.");

                    out_formatted.append(substitution);

                    ptrs_handled_count += 1;
                    head_ptr += (1 + 1); // We want to skip the '{' and the '}', i.e. 1 + 1

                    continue;
                }

                if (character_is_number(*look_ahead_ptr))
                {
                    std::string num_string;

                    for (; character_is_number(*look_ahead_ptr) and look_ahead_ptr != msg_end; look_ahead_ptr++)
                        num_string += *look_ahead_ptr; // incrementing look_ahead_ptr until we hit a non-number, or we hit the end of `msg`.

                    if (look_ahead_ptr == msg_end)
                    {
                        head_ptr += 1;
                        continue;
                    }

                    if (*look_ahead_ptr != '}')
                    { // That means that we found {...NUMS... without an ending '}', it wasn't a 
                      // substitution sequence ({...}), just continue on like nothing happened.
                        head_ptr += 1;
                        continue;
                    }

                    auto numeric_characters_to_data_index = atoi(num_string.data());

                    std::string substitution = Get_Format_Substitution(numeric_characters_to_data_index);

                    assert(substitution != failure_string && "Received a data type `Print` doesn't understand.");

                    out_formatted.append(substitution);

                    ptrs_handled_count += 1;
                    head_ptr += (1 + 1) + num_string.length();

                    continue;
                }
            }

            if (*head_ptr != '\0')
            {
                out_formatted += *head_ptr;
            }

            head_ptr += 1;
        }

        return out_formatted;
    }
};
