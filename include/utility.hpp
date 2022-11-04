#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <type_traits>
#include <concepts>
#include <utility>
#include <limits>
#include <memory>


namespace bu {

    using Usize = std::size_t;
    using Isize = std::make_signed_t<Usize>;

    template <class T> constexpr auto maximum = std::numeric_limits<T>::max();
    template <class T> constexpr auto minimum = std::numeric_limits<T>::min();

    inline constexpr struct InPlace {} in_place;


    // Add iterator support
    template <class T> [[nodiscard]]
    constexpr auto distance(T* const begin, T* const end)
        noexcept -> std::ptrdiff_t
    {
        return (end - begin);
    }

    template <std::input_iterator It, std::sentinel_for<It> S>
    constexpr auto unsigned_distance(It const begin,S const end)
        noexcept -> Usize
    {
        return static_cast<Usize>(distance(begin, end));
    }


    template <class T>
    constexpr auto swap(T& a, T& b)
        noexcept(noexcept(a.swap(b))) -> void
        requires requires { { a.swap(b) } -> std::same_as<void>; }
    {
        a.swap(b);
    }
    template <class T>
    constexpr auto swap(T& a, T& b)
        noexcept(std::is_nothrow_move_assignable_v<T>) -> void
        requires (!requires { a.swap(b); })
    {
        T c = std::move(a);
        a = std::move(b);
        b = std::move(c);
    }

    template <Usize n>
    struct [[nodiscard]] Metastring {
        char m_buffer[n];

        consteval Metastring(char const(&literal)[n]) noexcept {
            for (Usize i = 0; i != n; ++i) {
                m_buffer[i] = literal[i];
            }
        }
        constexpr auto string() const noexcept -> char const* {
            return m_buffer;
        }
    };

    template <Usize n>
    Metastring(char const(&)[n]) -> Metastring<n>;

}
