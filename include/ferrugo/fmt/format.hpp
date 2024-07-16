#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <ferrugo/core/overloaded.hpp>
#include <ferrugo/core/type_traits.hpp>
#include <ferrugo/fmt/buffer.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string_view>
#include <variant>
#include <vector>

namespace ferrugo
{
namespace fmt
{

template <class T, class = void>
struct formatter;

class parse_context
{
public:
    explicit parse_context(std::string_view specifier) : m_specifier{ specifier }
    {
    }

    std::string_view specifier() const
    {
        return m_specifier;
    }

private:
    std::string_view m_specifier;
};
class format_context
{
public:
    constexpr explicit format_context(buffer& os) : m_os{ os }
    {
    }

    buffer& output() const
    {
        return m_os;
    }

    void flush(std::ostream& os)
    {
        os << std::string_view(m_os.begin(), m_os.size());
        m_os.reset();
    }

private:
    buffer& m_os;
};

template <class... Args>
format_context& write_to(format_context& ctx, const Args&... args)
{
    (formatter<Args>{}.format(ctx, args), ...);
    return ctx;
}

struct format_error : std::runtime_error
{
    explicit format_error(std::string message) : std::runtime_error{ std::move(message) }
    {
    }
};

namespace detail
{

struct arg_ref
{
    using arg_printer = void (*)(format_context&, const void*, const parse_context&);
    arg_printer m_printer;
    const void* m_ptr;

    template <class T>
    explicit arg_ref(const T& item)
        : m_printer{ [](format_context& format_ctx, const void* ptr, const parse_context& parse_ctx)
                     {
                         formatter<T> f{};
                         f.parse(parse_ctx);
                         f.format(format_ctx, *static_cast<const T*>(ptr));
                     } }
        , m_ptr{ std::addressof(item) }
    {
    }

    arg_ref(const arg_ref&) = default;
    arg_ref(arg_ref&&) = default;

    void print(format_context& format_ctx, const parse_context& parse_ctx) const
    {
        m_printer(format_ctx, m_ptr, parse_ctx);
    }
};

template <class... Args>
auto wrap_args(const Args&... args) -> std::vector<arg_ref>
{
    std::vector<arg_ref> result;
    result.reserve(sizeof...(args));
    (result.push_back(arg_ref{ args }), ...);
    return result;
}

class format_string
{
private:
    struct print_text
    {
        std::string_view text;
    };

    struct print_argument
    {
        int index;
        parse_context context;
    };

    using print_action = std::variant<print_text, print_argument>;

public:
    explicit format_string(std::string_view fmt) : m_actions{ parse(fmt) }
    {
    }

    void format(format_context& format_ctx, const std::vector<arg_ref>& arguments) const
    {
        for (const auto& action : m_actions)
        {
            std::visit(
                ferrugo::core::overloaded{ [&](const print_text& a) { write_to(format_ctx, a.text); },
                                           [&](const print_argument& a)
                                           { arguments.at(a.index).print(format_ctx, a.context); } },
                action);
        }
    }

    auto format(const std::vector<arg_ref>& arguments) const -> std::string
    {
        buffer buf{};
        format_context format_ctx{ buf };
        format(format_ctx, arguments);
        return std::string(buf.begin(), buf.end());
    }

    friend std::ostream& operator<<(std::ostream& os, const format_string& item)
    {
        for (const auto& action : item.m_actions)
        {
            std::visit(
                ferrugo::core::overloaded{ [&](const print_text& a) { os << a.text; },
                                           [&](const print_argument& a)
                                           {
                                               if (a.context.specifier().empty())
                                               {
                                                   os << "{" << a.index << "}";
                                               }
                                               else
                                               {
                                                   os << "{" << a.index << ":" << a.context.specifier() << "}";
                                               }
                                           } },
                action);
        }
        return os;
    }

private:
    std::vector<print_action> m_actions;

    static auto parse(std::string_view fmt) -> std::vector<print_action>
    {
        static const auto is_opening_bracket = [](char c) { return c == '{'; };
        static const auto is_closing_bracket = [](char c) { return c == '}'; };
        static const auto is_bracket = [](char c) { return is_opening_bracket(c) || is_closing_bracket(c); };
        static const auto is_colon = [](char c) { return c == ':'; };
        std::vector<print_action> result;
        int arg_index = 0;
        while (!fmt.empty())
        {
            const auto begin = std::begin(fmt);
            const auto end = std::end(fmt);
            const auto bracket = std::find_if(begin, end, is_bracket);
            if (bracket == end)
            {
                result.push_back(print_text{ fmt });
                fmt = make_string_view(bracket, end);
            }
            else if (bracket[0] == bracket[1])
            {
                result.push_back(print_text{ make_string_view(begin, bracket + 1) });
                fmt = make_string_view(bracket + 2, end);
            }
            else if (is_opening_bracket(bracket[0]))
            {
                const auto closing_bracket = std::find_if(bracket + 1, end, is_closing_bracket);
                if (closing_bracket == end)
                {
                    throw format_error{ "unclosed bracket" };
                }
                result.push_back(print_text{ make_string_view(begin, bracket) });

                const auto [actual_index, fmt_specifer] = std::invoke(
                    [](std::string_view arg, int current_index) -> std::tuple<int, std::string_view>
                    {
                        const auto colon = std::find_if(std::begin(arg), std::end(arg), is_colon);
                        const auto index_part = make_string_view(std::begin(arg), colon);
                        const auto fmt_part = make_string_view(colon != std::end(arg) ? colon + 1 : colon, std::end(arg));
                        const auto index = !index_part.empty() ? parse_int(index_part) : current_index;
                        return { index, fmt_part };
                    },
                    make_string_view(bracket + 1, closing_bracket),
                    arg_index);
                result.push_back(print_argument{ actual_index, parse_context{ fmt_specifer } });
                fmt = make_string_view(closing_bracket + 1, end);
                ++arg_index;
            }
        }
        return result;
    }

    static auto make_string_view(std::string_view::iterator b, std::string_view::iterator e) -> std::string_view
    {
        if (b < e)
            return { std::addressof(*b), std::string_view::size_type(e - b) };
        else
            return {};
    }

    static auto parse_int(std::string_view txt) -> int
    {
        int result = 0;
        for (char c : txt)
        {
            assert('0' <= c && c <= '9');
            result = result * 10 + (c - '0');
        }
        return result;
    }
};

template <bool NewLine = false>
struct print_to_fn
{
    struct impl
    {
        std::ostream& m_os;
        format_string m_formatter;

        template <class... Args>
        void operator()(Args&&... args) const
        {
            buffer buf{};
            format_context format_ctx{ buf };
            m_formatter.format(format_ctx, wrap_args(std::forward<Args>(args)...));
            if constexpr (NewLine)
            {
                write_to(format_ctx, '\n');
            }

            format_ctx.flush(m_os);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << item.m_formatter;
        }
    };

    auto operator()(std::ostream& os, std::string_view fmt) const -> impl
    {
        return impl{ os, format_string{ fmt } };
    }

    auto operator()(std::string_view fmt) const -> impl
    {
        return impl{ std::cout, format_string{ fmt } };
    }
};

struct format_fn
{
    struct impl
    {
        format_string m_formatter;

        template <class... Args>
        auto operator()(Args&&... args) const -> std::string
        {
            return m_formatter.format(wrap_args(std::forward<Args>(args)...));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << item.m_formatter;
        }
    };

    auto operator()(std::string_view fmt) const -> impl
    {
        return impl{ format_string{ fmt } };
    }
};

struct join_fn
{
    template <class Iter>
    struct impl
    {
        Iter m_begin;
        Iter m_end;
        std::string_view m_separator;
    };

    template <class Range>
    auto operator()(Range&& range, std::string_view separator) const -> impl<core::iterator_t<Range>>
    {
        return impl<core::iterator_t<Range>>{ std::begin(range), std::end(range), separator };
    }
};

}  // namespace detail

template <class T>
struct ostream_formatter
{
    void parse(const parse_context&)
    {
    }

    void format(format_context& ctx, const T& item) const
    {
        std::stringstream ss;
        ss << item;
        const std::string s = ss.str();
        ctx.output().append(s.data(), s.size());
    }
};

template <char... Fmt>
struct sprintf_formatter
{
    void parse(const parse_context&)
    {
    }

    template <class T>
    void format(format_context& ctx, T item) const
    {
        static const char fmt[] = { '%', Fmt..., '\0' };
        char buffer[64];
        int chars_written = std::sprintf(buffer, fmt, item);
        ctx.output().append(buffer, chars_written);
    }
};

template <class T>
struct formatter<T, std::enable_if_t<std::is_integral_v<T>>>
{
    void parse(const parse_context&)
    {
    }

    void format(format_context& ctx, T item) const
    {
        const std::string s = std::to_string(item);
        ctx.output().append(s.data(), s.size());
    }
};

template <>
struct formatter<int> : sprintf_formatter<'d'>
{
};

template <>
struct formatter<long> : sprintf_formatter<'l', 'd'>
{
};

template <>
struct formatter<long long> : sprintf_formatter<'l', 'l', 'd'>
{
};

template <>
struct formatter<unsigned> : sprintf_formatter<'u'>
{
};

template <>
struct formatter<unsigned long> : sprintf_formatter<'l', 'u'>
{
};

template <>
struct formatter<unsigned long long> : sprintf_formatter<'l', 'l', 'u'>
{
};

template <>
struct formatter<float> : sprintf_formatter<'f'>
{
};

template <>
struct formatter<double> : sprintf_formatter<'f'>
{
};

template <>
struct formatter<long double> : sprintf_formatter<'L', 'f'>
{
};

template <>
struct formatter<std::string>
{
    void parse(const parse_context&)
    {
    }

    void format(format_context& ctx, const std::string& item) const
    {
        ctx.output().append(item.data(), item.size());
    }
};

template <>
struct formatter<std::string_view>
{
    void parse(const parse_context&)
    {
    }

    void format(format_context& ctx, std::string_view item) const
    {
        ctx.output().append(item.data(), item.size());
    }
};

template <>
struct formatter<const char*>
{
    void parse(const parse_context&)
    {
    }

    void format(format_context& ctx, const char* item) const
    {
        ctx.output().append(item, std::strlen(item));
    }
};

template <>
struct formatter<char>
{
    void parse(const parse_context&)
    {
    }

    void format(format_context& ctx, const char item) const
    {
        ctx.output().append(&item, 1);
    }
};

template <std::size_t N>
struct formatter<char[N]>
{
    void parse(const parse_context&)
    {
    }

    void format(format_context& ctx, const char (&item)[N]) const
    {
        ctx.output().append(item, N - 1);
    }
};

template <>
struct formatter<bool>
{
    void parse(const parse_context&)
    {
    }

    void format(format_context& ctx, bool item) const
    {
        if (item)
        {
            write_to(ctx, "true");
        }
        else
        {
            write_to(ctx, "false");
        }
    }
};

template <class Iter>
struct formatter<detail::join_fn::impl<Iter>>
{
    void parse(const parse_context&)
    {
    }

    void format(format_context& ctx, const detail::join_fn::impl<Iter>& item) const
    {
        auto it = item.m_begin;
        auto end = item.m_end;
        if (it == end)
        {
            return;
        }
        write_to(ctx, *it++);
        for (; it != end; ++it)
        {
            write_to(ctx, item.m_separator, *it);
        }
    }
};

static constexpr inline auto join = detail::join_fn{};

static constexpr inline auto print = detail::print_to_fn<>{};
static constexpr inline auto println = detail::print_to_fn<true>{};

static constexpr inline auto format = detail::format_fn{};

}  // namespace fmt

}  // namespace ferrugo
