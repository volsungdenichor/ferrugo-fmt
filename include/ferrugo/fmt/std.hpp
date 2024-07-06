#pragma once

#include <ferrugo/fmt/format.hpp>
#include <optional>
#include <tuple>
#include <vector>

namespace ferrugo
{

namespace fmt
{

template <class Range>
struct range_formatter
{
    void parse(const parse_context&)
    {
    }

    void format(format_context& os, const Range& item) const
    {
        const auto b = std::begin(item);
        const auto e = std::end(item);
        write_to(os, "[");
        for (auto it = b; it != e; ++it)
        {
            if (it != b)
            {
                write_to(os, ", ");
            }
            write_to(os, *it);
        }
        write_to(os, "]");
    }
};

template <class... Args>
struct formatter<std::vector<Args...>> : range_formatter<std::vector<Args...>>
{
};

template <class Tuple>
struct tuple_formatter
{
    void parse(const parse_context&)
    {
    }

    void format(format_context& ctx, const Tuple& item) const
    {
        write_to(ctx, "(");
        std::apply(
            [&](const auto&... args)
            {
                auto n = 0u;
                (write_to(ctx, args, (++n != sizeof...(args) ? ", " : "")), ...);
            },
            item);
        write_to(ctx, ")");
    }
};

template <class F, class S>
struct formatter<std::pair<F, S>> : tuple_formatter<std::pair<F, S>>
{
};

template <class... Args>
struct formatter<std::tuple<Args...>> : tuple_formatter<std::tuple<Args...>>
{
};

template <class T>
struct formatter<std::optional<T>>
{
    formatter<T> m_inner = {};

    void parse(const parse_context& ctx)
    {
        m_inner.parse(ctx);
    }

    void format(format_context& os, const std::optional<T>& item) const
    {
        if (item)
        {
            write_to(os, "some(");
            m_inner.format(os, *item);
            write_to(os, ")");
        }
        else
        {
            write_to(os, "none");
        }
    }
};

template <class T>
struct formatter<std::reference_wrapper<T>>
{
    formatter<std::remove_const_t<T>> m_inner = {};

    void parse(const parse_context& ctx)
    {
        m_inner.parse(ctx);
    }

    void format(format_context& os, const std::reference_wrapper<T>& item) const
    {
        m_inner.format(os, item.get());
    }
};

}  // namespace fmt
}  // namespace ferrugo