#include "strong_type.h"

#include <iostream>

// commutative_addition
// divisible
// invokable
// hashable
// serializable

// safe length unit => underlying value is in meter (should be carved in the code, not in a comment)
struct length_unit : wit::strong_type<
    float           // underlying type
    , length_unit   // unique tag (thank to CRTP, it is the type itself)
    , wit::comparable
    , wit::explicitly_convertible_to<float>::modifier
    , wit::self_addable
    , wit::stringable
    >
{
    struct meter;
    using strong_type::strong_type;
    length_unit() : strong_type{ 0 } {}
};

// struct milliseconds;
// struct seconds;
// struct minutes;

struct time_unit : wit::strong_type<
    long long
    , time_unit
    , wit::comparable
    , wit::incrementable
    //, from_to<milliseconds, 1>
    //, from_to<seconds, 1000>
    //, from_to<minutes, 60000>
    >
{
    using strong_type::strong_type;
    time_unit() : strong_type{ 0 } {}
};

// a 3d vector without any semantic
template<typename T>
struct vector3
{
    vector3(T _x, T _y, T _z) : x{std::move(_x)}, y{std::move(_y)}, z{std::move(_z)} {}
    bool operator==(const vector3& _rhs) const { return x == _rhs.x && y == _rhs.y && z == _rhs.z; }
    bool operator!=(const vector3& _rhs) const { return !(*this == _rhs); }
    T x, y, z;
};

/*
struct position;

struct timespan;

struct velocity;
*/
using displacement = wit::strong_type<
    vector3<length_unit> // underlying type
    , struct displacement_tag // unique tag
    , wit::equalable // displacement can use the underlying type comparison operators
    //, commutative_addition<position, position>, // displacement + position = position + displacement => give a position
    //, divisible<timespan, velocity> // displacement divide by timespan gives velocity
>;

using namespace std;

struct stats
{
    unsigned int test_count = 0;
    unsigned int success_count = 0;

    void Check(bool _condition, const char* _failure_message)
    {
        test_count++; 
        if (!_condition)
        {
            cout << _failure_message << "**ERROR**" << endl;
        }
        else
        {
            success_count++;
        }
    }
};

template<typename T>
stats test(stats _stats, const T& a, const T& b)
{
    #define CHECK(OP) _stats.test_count++; if ((a.get() OP b.get())!=(a OP b)) { cout << a.get() << #OP << b.get() << " returned " << (a OP b) << "**ERROR**" << endl; } else { _stats.success_count++; }
    CHECK(==);
    CHECK(!=);
    CHECK(<);
    CHECK(>);
    CHECK(<=);
    CHECK(>=);
    #undef CHECK
    return _stats;
};

int main()
{
    stats stats;

    cout << boolalpha;

    stats = test(stats, length_unit{ 17 }, length_unit{ 42 });
    stats = test(stats, length_unit{ 23 }, length_unit{ 23 });

    stats = test(stats, time_unit{ 17 }, time_unit{ 42 });
    stats = test(stats, time_unit{ 23 }, time_unit{ 23 });
    stats.Check(float{ length_unit{ 5 } }==5, "float{ length_unit{ 5 } }==5");
    stats.Check(length_unit{ 4 } + length_unit{ 7 }==length_unit{ 11 }, "length_unit{ 4 } + length_unit{ 7 }==length_unit{ 11 }");
    stats.Check(std::to_string(length_unit{ 18 }) == std::to_string(18.f), "std::to_string(length_unit{ 18 }) == std::to_string(18.f)");
    { auto t = time_unit{37}; stats.Check(++t == time_unit{38}, "pre increment"); }
    { auto t = time_unit{37}; stats.Check(t++ == time_unit{37} && t == time_unit{38}, "post increment"); }
    { auto t = time_unit{37}; t += time_unit{17}; stats.Check(t == time_unit{54}, "addition assignment"); }

    {
        auto v0 = vector3<length_unit>{ length_unit{1}, length_unit{2}, length_unit{3} };
        auto v1 = vector3<length_unit>{ length_unit{1}, length_unit{2}, length_unit{3} };
        auto v2 = vector3<length_unit>{ length_unit{4}, length_unit{5}, length_unit{6} };
        stats.Check(v0==v1,"vector3 equal");
        stats.Check(v1!=v2,"vector3 different");
    }
    cout << stats.success_count << " success over " << stats.test_count << " tests." << endl;
}