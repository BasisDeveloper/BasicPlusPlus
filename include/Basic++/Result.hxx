/* Mission: Is to create "zero" cost abstarction for error reporting.
            Using an optimizing compiler the use of these structures
            should result in very VERY low cost, if not zero. */

#pragma once

#include <utility>
#include <source_location>
#include <cstdlib>

namespace Basic {
  inline namespace ResultType {
    template<typename T, bool implicit_failure = false>
    struct Result;
  }
}

#include "Basic++/Expectations.hxx"

namespace Basic
{
  static_assert(sizeof(void*) == 8); // make sure we're on a 64 bit CPU.

  constexpr auto Size_Of_Platform_Pointer = sizeof(void*);

  // 0b1101110011001101 (0xDCCD) is also a good value.
  constexpr static std::uint64_t Error_Status_Flags = 0xDEAD;

  thread_local static char* error_message_chain[std::numeric_limits<std::uint16_t>::max()] = {};

  // the virutal address space of a process by default in windows is 32 bits
  // and can only ever be 32 bits, it is an operating system invariant, it is a core
  // assumption of the Windows OS. Meaning that we only need a 48 bit pointer to 
  // store any refernces to readonly strings in the .rdata section.
  // https://stackoverflow.com/questions/16198700/%20using-the-extra-16-bits-in-64-bit-pointers
  struct alignas(Size_Of_Platform_Pointer) Pointer48
  {
    union
    {
      std::uint64_t pointer_48 = { };
      std::uint16_t free_bits;
    };

    inline auto as_ptr() -> void* { return (void*)(pointer_48 >> 16); }

    inline auto as_ptr() const -> const void* { return (void*)(pointer_48 >> 16); }

    // this generates more optimal code
    inline Pointer48(const void* const src_ptr, std::uint64_t free_bits_value = 0)
      : pointer_48(((std::uint64_t)src_ptr << 16) | free_bits_value) {
    }
  }; static_assert(sizeof(Pointer48) == sizeof(std::uint64_t));

  inline namespace ResultType
  {
    using string_type = const char*;

    static string_type AOK = "^(AOK)";

    template<typename T, bool implicit_failure>
    struct Result
    {
    private:
      union
      {
        Pointer48 _status;
        T _value;
      };

    public:
      using Type = T;
      constexpr const static bool is_failure = implicit_failure;

      [[nodiscard]] inline string_type status() const
      {
        return (string_type)_status.as_ptr();
      }

      [[nodiscard]] inline T& value() { return _value; }
      [[nodiscard]] inline const T& value() const { return _value; }

      Result() = default;

      // a conversion constructor for Result classes 
      // that may not be the same, but their T types are
      // convertable to one another. 
      //  i.e. `Result<float>` is convertable to `Result<int>`
      //       so this makes that conversion possible.
      template<typename U = T>
      Result<T, false>(const Result<U, false>&& other) noexcept
      {
          // TODO:
      };

      template<typename U = T>
      Result<T, false>(const Result<U, true>&& other) noexcept
      {

      }

      Result(T&& _value) requires(implicit_failure == false) : _value(std::move(_value)) {}

      Result(const T& _value) requires(implicit_failure == false) : _value(_value) {}

      Result(string_type msg) : _status(msg, Error_Status_Flags) {}

      [[nodiscard]] T* operator->()
      {
        EXPECT((this->operator bool()) == true,
          "you mustn't dereference an invalid Result<T>, "
          "or any other kind for that matter.");

        return &_value;
      }

      [[nodiscard]] T& operator* ()
      {
        EXPECT((this->operator bool()) == true,
          "you mustn't dereference an invalid Result<T>, "
          "or any other kind for that matter.");
        return _value;
      }

      [[nodiscard]] const T& operator* () const
      {
        EXPECT((this->operator bool()) == true,
          "you mustn't dereference an invalid Result<T>, "
          "or any other kind for that matter.");
        return _value;
      }

      // [[nodiscard]] 
      inline auto expect(
        const char* msg = nullptr,
        std::source_location sl = std::source_location::current()) -> T&
      {
          // am I playing with fire here?
        #ifndef NO_EXPECTATIONS
        if (!(this->operator bool())) [[likely]]
        {
        // TODO: make a version of expect that can chain messages into one string.

          if (msg)
            Basic::Expectations::Expect(false, msg, sl);
          else
          {
            // TODO: this is dumb, we shouldn't have to construct a message like this.
            const char* status_ptr = (const char*)_status.as_ptr();

            if (status_ptr == nullptr) status_ptr = "<(<undefined error message, this should be impossible>)>";

            Basic::Message message(status_ptr);

            Basic::Expectations::Expect(false, std::move(message), sl);
          }

          // before we debug break we want to dump anything buffered 
          // we have in stdout so the user can see whatever is in there.
          std::fflush(stdout);

          std::exit(EXIT_FAILURE);
        }
        else
        {
          return _value;
        }
        #else
        return value;
        #endif
      }

      //[[nodiscard]]
      inline auto expect(
        const char* msg = nullptr,
        std::source_location sl = std::source_location::current()) const -> const T&
      {
        return expect(msg, sl);
      }

      constexpr inline auto ok() const -> bool
      {
        return _status.free_bits != Error_Status_Flags;
      }

      // for some silly reason I can't compare the pointers...
      constexpr inline operator bool() const { return ok(); }

      ~Result() {};

      auto as_error() const -> Result<T, true>
      {
        return *this;
      }

      Result<T, true> operator~() const
      {
        Result<T, true> r = Result<T, true>(this->status());
        return r;
      }
    }; static_assert(sizeof(Result<int>) == 8);

  // ========================           ========================
  // ======================== REFERENCE ========================
  // ========================           ========================

    template<typename T>
    struct Result<T&>
    {
    private:
      union
      {
        Pointer48 _status;
        // "lying" to the user that a reference will be used is... interseting.
        // but under the hood references are just pointers, so...
        T* _value = {};
      };

    public:
      using Type = T;

      string_type status() const { return (string_type)_status.as_ptr(); }

      /* in the case of references take the value without performing a check is quite dangerous,
         and may EASILY result in crash if mishandled. This concern warrants a name change to
         signify the danger.*/
      [[nodiscard]] auto unsafe_value() -> T& { return *_value; }
      [[nodiscard]] auto unsafe_value() const ->  const T& { unsafe_value(); }

      Result() = default;

      Result(T& _value) : _value(&_value) {}

      // yes, this is undefined behavior, yes, I mean to do it.
      // even if we fail we MUST initialize `value` to something, so this is my solution.
      // if the user receives a Result that has a failure value, then they shouldn't dereference
      // it anyway, if they do, they'll recieved an expection for their carelessness.
      Result(string_type msg) : _status(msg, Error_Status_Flags) {}

      [[nodiscard]] T& operator* ()
      {
        EXPECT((this->operator bool()) == true,
          "you mustn't dereference an invalid Result<T&>, "
          "or any other kind for that matter.");

        return _value;
      }

      //[[nodiscard]] 
      inline auto expect(
        const char* msg = nullptr,
        std::source_location sl = std::source_location::current()) -> T&
      {
        // am I playing with fire here?
        #ifndef NO_EXPECTATIONS
        if (!(this->operator bool())) [[likely]]
        {
          // TODO: make a version of expect that can chain messages into one string.

          if (msg)
            Basic::Expectations::Expect(false, msg, sl);
          else
          {
            // TODO: this is dumb, we shouldn't have to construct a message like this.
            const char* status_ptr = (const char*)_status.as_ptr();
            Basic::Message message(status_ptr);
            Basic::Expectations::Expect(false, std::move(message), sl);
          }

        // before we debug break we want to dump anything buffered 
        // we have in stdout so the user can see whatever is in there.
          std::fflush(stdout);

          BASIC_DEBUG_BREAK();

          std::exit(EXIT_FAILURE);
        }
        else
        {
          return *_value;
        }
        #else
        return value;
        #endif
      }

      //[[nodiscard]] 
      inline auto expect(
        const char* msg = nullptr,
        std::source_location sl = std::source_location::current()) const -> const T&
      {
        return expect(msg, sl);
      }

      constexpr operator bool() const { return _status.free_bits != Error_Status_Flags; }
    }; static_assert(sizeof(Result<int&>) == 8);

    /* ========================      ======================== */
    /* ======================== BOOL ======================== */
    /* ========================      ======================== */

    /*
     this structures enables return value like:
        `return false;`
        or:
        `return { false, "this is an error message..." };
        or even:
        `return { true, "this is a success message!" };
    */

    template<>
    struct Result<bool>
    {
    private:
      const bool _value = false;
      string_type _status = AOK;
    public:
      using Type = bool;

      [[nodiscard]] const bool& value() { return _value; }

      string_type status() const { return _status; }

      Result() = default;

      Result(string_type msg) : _status(msg) {}

      Result(bool&& _value) : _value(_value) {}

      Result(const bool& _value) : _value(_value) {}

      Result(bool&& _value, string_type msg) : _value(_value), _status(msg) {}

      const bool& operator* () { return value(); }

      operator bool() const { return _status[0] == AOK[0]; }

      //[[nodiscard]] 
      inline auto expect(
        const char* msg = nullptr,
        std::source_location sl = std::source_location::current()) -> const bool
      {
        // am I playing with fire here?
        #ifndef NO_EXPECTATIONS
        if (!(this->operator bool())) [[likely]]
        {
          // TODO: make a version of expect that can chain messages into one string.

          if (msg)
            Basic::Expectations::Expect(false, msg, sl);
          else
            Basic::Expectations::Expect(false, _status, sl);

          // before we debug break we want to dump anything buffered 
          // we have in stdout so the user can see whatever is in there.
          std::fflush(stdout);

          BASIC_DEBUG_BREAK();

          std::exit(EXIT_FAILURE);
        }
        else
        {
          return _value;
        }
        #else
        return value;
        #endif
      }
    };

    using Err = Result<bool>;

    constexpr static bool Success = true;
    constexpr static bool Failure = false;
  }
}