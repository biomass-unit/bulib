#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>

#include <type_traits>
#include <functional>
#include <concepts>
#include <typeinfo>
#include <utility>
#include <compare>
#include <limits>
#include <memory>

#define BU ::bu::


namespace bu {

    using Usize = std::size_t;
    using Isize = std::make_signed_t<Usize>;

    template <class T> constexpr T maximum = std::numeric_limits<T>::max();
    template <class T> constexpr T minimum = std::numeric_limits<T>::min();


    struct InPlace {};
    constexpr InPlace in_place;

    template <class>
    struct InPlaceType {};
    template <class T>
    constexpr InPlaceType<T> in_place_type;


    template <class>
    constexpr bool always_false = false;


    template <class T>
    concept nothrow_movable = std::is_nothrow_move_constructible_v<T>
        && std::is_nothrow_move_assignable_v<T>;
    template <class T>
    concept nothrow_copyable = std::is_nothrow_copy_constructible_v<T>
        && std::is_nothrow_copy_assignable_v<T>;


    [[noreturn]]
    inline auto unreachable() {
        std::puts("A branch marked as unreachable was reached");
        std::terminate();
    }


    template <std::input_iterator It, std::sentinel_for<It> Se> [[nodiscard]]
    constexpr auto distance(It begin, Se const end)
        noexcept -> std::ptrdiff_t
    {
        if constexpr (std::random_access_iterator<It>) {
            return end - begin;
        }
        else {
            std::ptrdiff_t dist = 0;
            for (; begin != end; ++begin, ++dist);
            return dist;
        }
    }

    template <std::input_iterator It, std::sentinel_for<It> S>
    constexpr auto unsigned_distance(It const begin,S const end)
        noexcept -> Usize
    {
        return static_cast<Usize>(BU distance(begin, end));
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

    template <class T>
    constexpr auto exchange(T& reference, std::type_identity_t<T>&& new_value)
        noexcept(nothrow_movable<T>) -> T
    {
        T old_value = std::move(reference);
        reference = std::move(new_value);
        return old_value;
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


    template <class... Ts>
    struct Typelist {
        static constexpr Usize size = sizeof...(Ts);

        template <template <class...> class Trait>
        static constexpr bool all =
            std::conjunction_v<Trait<Ts>...>;

        template <template <class...> class Trait>
        static constexpr bool any =
            std::disjunction_v<Trait<Ts>...>;

        template <template <class...> class Trait>
        static constexpr bool none =
            std::conjunction_v<std::negate<Trait<Ts>>...>;
    };

}
