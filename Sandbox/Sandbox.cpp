// #define NO_EXPECTATIONS

#include "Basic++/Expectations.hxx"
using namespace Basic::Printing;
using namespace Basic::ResultType;

#include "Basic++/Result.hxx"

#include "Basic++/Printing.hxx"

#include "Basic++/defer.hxx"

#include "Basic++/dbg.hxx"

auto An_Result_Of_Int(int v) -> Result<int>
{
    if (v > 5)
        return { __FUNCTION__ };
    return 5;
}

auto An_Result_Of_Const_Int_Reference(const int& v) -> Result<decltype(v)>
{
    if (v > 5)
        return { __FUNCTION__ };
    return v;
}

auto An_Result_Of_Int_Reference(int& v) -> Result<decltype(v)>
{
    if (v > 5)
        return { __FUNCTION__ };
    return v;
}

auto An_Result_Of_Newed_Int_Reference(int v) -> Result<int&>
{
    if (v > 5)
        return { __FUNCTION__ };

    int* new_int = new int{ v };

    return *new_int;
}

auto An_Result_Of_String(std::string v) -> Result<std::string>
{
    if (v == "fail")
        return { __FUNCTION__ };

    return v;
}

auto An_Result_Of_String_Reference(std::string& v) -> Result<std::string&>
{
    if (v == "fail")
        return { __FUNCTION__ };

    return v;
}

class NTT
{
public:
    int value = {};
    bool fail = false;

    NTT(const NTT& other)
    {
        this->value = other.value;
        this->fail = other.fail;
        Println("NTT move constructed!");
    }

    NTT(const NTT&& other) noexcept
    {
        this->value = other.value;
        this->fail = other.fail;
        Println("NTT move constructed!");
    }

    NTT()
    {
        Println("NTT constructed!");
    }
    ~NTT()
    {
        Println("NTT destructed!");
    }
}; static_assert(!std::is_trivial_v<NTT>);

auto An_Result_Of_A_Non_Trival_Type(NTT n) -> Result<NTT>
{
    if (n.fail)
        return "failure requested!";

    return n;
}

auto An_Result_Of_A_Non_Trival_Type_Ref(NTT& n) -> Result<NTT&>
{
    if (n.fail)
        return "failure requested!";

    return n;
}

static_assert(sizeof(Result<int>) == 8);
static_assert(sizeof(Result<int&>) == 8);
static_assert(sizeof(Result<int*>) == 8);
static_assert(sizeof(Result<bool>) == 16);

auto some_bool_fn() -> Err
{
    return false;
}

auto append_69(const char* buffer, std::size_t buffer_size = (std::size_t)-1) -> Result<char*>
{
    if (buffer_size == (std::size_t)-1)
        buffer_size = std::strlen(buffer);

    char* append_str = new char[buffer_size + 3];

    std::memcpy(append_str, buffer, buffer_size);

    append_str[buffer_size] = '6';
    append_str[buffer_size + 1] = '9';
    append_str[buffer_size + 2] = '\0';

    return append_str;
}

/* a dummy class to test how an API would look with Result<T> at the forefront.*/
class StringBuilder
{
    std::vector<const char*> strings_buffer;
public:
    auto append(const char* str) -> Err
    {
        strings_buffer.emplace_back(str);

        return Basic::Success;
    }

    auto build() -> Err
    {
        // TODO:
        return Basic::Success;
    }
};

int main()
{
    bool condition = true;

    // SUPER COOL!
    dbg if (condition)
    {
        // Basic::DbgPrintln("<this prints only in debug>");
        Basic::Println("<this prints only in debug (with the DBG_NO_DBG)>");
    }

    // SUPER COOL!
    dbg Basic::Println("<this also only prints in debug builds (with the DBG_NO_DBG)>");

    #if 0
    Println("{}", sizeof(Result<int>));

    Println("Hello, From Sandbox.exe!");

    Println(An_Result_Of_Int(5).expect());

    int integer_to_referenced = 5;

    Println(An_Result_Of_Const_Int_Reference(integer_to_referenced).expect());

    Println(An_Result_Of_Newed_Int_Reference(5).expect());

    An_Result_Of_String("Pass!").expect();

    std::string str = "pass!";

    auto str_ref = An_Result_Of_String(str).expect();

    assert((str_ref == "pass!"));

    integer_to_referenced = 10;

    Println("!!! Failing On Purpose !!!");
    auto& invalid_ref = *An_Result_Of_Int_Reference(integer_to_referenced);

    invalid_ref = 4;

    Println(An_Result_Of_Int(6).expect());
    #endif

    // copy:
    Println("copy:"); {
        // look at the logs, you can see why using Result<T&> is better
        NTT ntt;
        ntt.value = 69;
        assert(An_Result_Of_A_Non_Trival_Type(ntt).expect().value == 69);
    }
    // ref:
    Println("ref:"); {
        NTT ntt;
        ntt.value = 69;
        assert(An_Result_Of_A_Non_Trival_Type_Ref(ntt).expect().value == 69);
    }
    // fail on purpose:
    {
        NTT ntt;
        ntt.fail = true;
        auto Result_ntt = An_Result_Of_A_Non_Trival_Type(ntt);

        if (!Result_ntt)
            Println(Result_ntt.status());
    }

    {
        auto rtnv = some_bool_fn();

        auto b = rtnv.expect();

        if (!rtnv)
        {
            Println("`some_bool_fn` failed with status '{}'", rtnv.status());
        }
    }

    {
        auto str = append_69("what's up, the number is ");

        const auto new_str = str.expect("couldn't append 69 to string");

        Println(new_str);
    }

    // value type semantics
    {
        Result<int> result(5);

        auto a = result.expect();

        EXPECT(a == 5, "a should've equalved 5");
    }

    // reference type semantics
    {
        // success
        {
            int a = 69;

            Result<int&> result(a);

            int& b = result.expect("attempted to call `.expect()` failed, it should've have.");

            b = 5;

            /* The semantics of `value()` on a reference type is strange, or in fact on any
               type of Result<T>. For instance, what does it mean to call `value()` on an
               invalid Result<T>? why should I be able to bypass the check of validity?
               If the reference is invalid then this function will cause a segfault. */
            result.unsafe_value() = 7;

            EXPECT(result.unsafe_value() == 7, "expected result.unsafe_value() to equal 7.");

            Println("result.unsafe_value() == {}", result.unsafe_value());
        }
        // failure
        {
            int a = 69;

            Result<int&> result("meant to fail, but you won't see this message");

            EXPECT(result == false, "expected failure.");
        }
    }

    //integer >> floating >> Basic::expect();

    auto a = An_Result_Of_Int(6).expect("failure, y'all! An expected one!");
    
    Println("Bye, bye, from Sandbox.exe!");
}