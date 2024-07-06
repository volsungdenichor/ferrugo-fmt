#include <catch2/catch_test_macros.hpp>
#include <ferrugo/fmt/fmt.hpp>
#include <ferrugo/core/ostream_utils.hpp>

#include "matchers.hpp"

using namespace std::string_view_literals;

using namespace ferrugo;

TEST_CASE("format - no explicit indices", "[format]")
{
    REQUIRE_THAT(core::str(fmt::detail::format_string("{} has {}.")), matchers::equal_to("{0} has {1}."));
}

TEST_CASE("format - some explicit indices", "[format]")
{
    REQUIRE_THAT(core::str(fmt::detail::format_string("{1} has {}.")), matchers::equal_to("{1} has {1}."));
}

TEST_CASE("format - all explicit indices", "[format]")
{
    REQUIRE_THAT(core::str(fmt::detail::format_string("{1} has {0}.")), matchers::equal_to("{1} has {0}."));
}

TEST_CASE("format - argument format specifiers", "[format]")
{
    REQUIRE_THAT(core::str(fmt::detail::format_string("{:abc} has {:def}.")), matchers::equal_to("{0:abc} has {1:def}."));
}

TEST_CASE("format - explicit indices and argument format specifiers", "[format]")
{
    REQUIRE_THAT(core::str(fmt::detail::format_string("{1:abc} has {0:def}.")), matchers::equal_to("{1:abc} has {0:def}."));
}

TEST_CASE("format", "")
{
    REQUIRE_THAT(  //
        fmt::format("{} has {}.")("Alice", "a cat"),
        matchers::equal_to("Alice has a cat."sv));
}

TEST_CASE("format - booolean", "")
{
    REQUIRE_THAT(  //
        fmt::format("{}-{}")(true, false),
        matchers::equal_to("true-false"sv));
}

TEST_CASE("format - char array", "")
{
    REQUIRE_THAT(  //
        fmt::format("{}-{}")("ABC", "DEF"),
        matchers::equal_to("ABC-DEF"sv));
}

TEST_CASE("format - string_view", "")
{
    REQUIRE_THAT(  //
        fmt::format("{}-{}")("ABC"sv, "DEF"sv),
        matchers::equal_to("ABC-DEF"sv));
}

TEST_CASE("format - a vector", "")
{
    REQUIRE_THAT(  //
        fmt::format("{} has the following animals: {}.")("Alice", std::vector{ "a cat", "a dog" }),
        matchers::equal_to("Alice has the following animals: [a cat, a dog]."sv));
}

TEST_CASE("format - basic types", "")
{
    REQUIRE_THAT(  //
        fmt::format("int={}, short={}, char={}, bool={}, float={}, double={}")(42, static_cast<short>(100), 'A', true, 3.14F, 3.14),
        matchers::equal_to("int=42, short=100, char=A, bool=true, float=3.140000, double=3.140000"sv));
}

TEST_CASE("print", "")
{
    std::stringstream ss;
    fmt::print(ss, "{} has {}.")("Alice", "a cat");
    REQUIRE_THAT(ss.str(), matchers::equal_to("Alice has a cat."sv));
}

TEST_CASE("println", "")
{
    std::stringstream ss;
    fmt::println(ss, "{} has {}.")("Alice", "a cat");
    REQUIRE_THAT(ss.str(), matchers::equal_to("Alice has a cat.\n"sv));
}

TEST_CASE("join", "")
{
    REQUIRE_THAT(
        fmt::format("{} has {}.")("Alice", fmt::join(std::vector{ "a cat", "a dog", "a turtle" }, ", ")),
        matchers::equal_to("Alice has a cat, a dog, a turtle."sv));
}
