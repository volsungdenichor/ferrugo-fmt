#pragma once

#include <algorithm>
#include <memory>

namespace ferrugo
{

namespace fmt
{

template <class T>
struct basic_buffer
{
    using grow_function_type = std::size_t (*)(std::size_t);
    std::size_t m_size;
    std::size_t m_capacity;

    grow_function_type m_grow_fn;
    std::unique_ptr<T[]> m_data;

    explicit basic_buffer(std::size_t capacity, grow_function_type grow_fn)
        : m_size{ 0 }
        , m_capacity{ capacity }
        , m_grow_fn{ grow_fn }
        , m_data{}
    {
        m_data.reset(new T[m_capacity]);
    }

    explicit basic_buffer() : basic_buffer(64, [](std::size_t n) { return 2 * n; })
    {
    }

    std::size_t size() const
    {
        return m_size;
    }

    const T* begin() const
    {
        return m_data.get();
    }

    const T* end() const
    {
        return begin() + size();
    }

    T* begin()
    {
        return m_data.get();
    }

    T* end()
    {
        return begin() + size();
    }

    void ensure_capacity(std::size_t required_capacity)
    {
        if (required_capacity <= m_capacity)
        {
            return;
        }

        std::size_t new_capacity = m_capacity;
        while (new_capacity < required_capacity)
        {
            new_capacity = m_grow_fn(new_capacity);
        }

        std::unique_ptr<T[]> ptr(new T[new_capacity]);
        std::copy(begin(), end(), ptr.get());
        m_data = std::move(ptr);
        m_capacity = new_capacity;
    }

    void append(const T* b, const T* e)
    {
        std::size_t s = std::distance(b, e);
        ensure_capacity(size() + s);
        std::copy(b, e, end());
        m_size += s;
    }

    void append(const T* b, std::size_t n)
    {
        append(b, b + n);
    }

    void reset()
    {
        m_size = 0;
    }
};

using buffer = basic_buffer<char>;

}  // namespace fmt
}  // namespace ferrugo
