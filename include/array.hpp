#pragma once

#include "option.hpp"
#include "utility.hpp"
#include "concepts.hpp"
#include "exception.hpp"


namespace bu {
    template <class T, Usize n>
    struct [[nodiscard]] Array {
        T m_array[n];

        using ContainedType = T;
        using Iterator      = T*;
        using Sentinel      = Iterator;
        using ConstIterator = T const*;
        using ConstSentinel = ConstIterator;
    
        [[nodiscard]]
        constexpr auto is_empty() const noexcept -> std::false_type {
            return {};
        }
        [[nodiscard]]
        constexpr auto size() const noexcept -> Usize {
            return n;
        }
        [[nodiscard]]
        constexpr auto data() const noexcept -> T const* {
            return m_array;
        }
        [[nodiscard]]
        constexpr auto data() noexcept -> T* {
            return m_array;
        }
        [[nodiscard]]
        constexpr auto front() const noexcept -> T const& {
            return m_array[0];
        }
        [[nodiscard]]
        constexpr auto front() noexcept -> T& {
            return m_array[0];
        }
        [[nodiscard]]
        constexpr auto back() const noexcept -> T const& {
            return m_array[n - 1];
        }
        [[nodiscard]]
        constexpr auto back() noexcept -> T& {
            return m_array[n - 1];
        }
        [[nodiscard]]
        constexpr auto begin() const noexcept -> T const* {
            return m_array;
        }
        [[nodiscard]]
        constexpr auto begin() noexcept -> T* {
            return m_array;
        }
        [[nodiscard]]
        constexpr auto end() const noexcept -> T const* {
            return m_array + n;
        }
        [[nodiscard]]
        constexpr auto end() noexcept -> T* {
            return m_array + n;
        }

        [[nodiscard]]
        constexpr auto operator[](Usize const index) const -> T const& {
            if (index < n)
                return m_array[index];
            else
                throw OutOfRange {};
        }
        [[nodiscard]]
        constexpr auto operator[](Usize const index) -> T& {
            return const_cast<T&>(const_cast<Array const&>(*this)[index]);
        }

        [[nodiscard]]
        constexpr auto at(Usize const index) const -> Option<T const&> {
            if (index < n)
                return m_array[index];
            else
                return nullopt;
        }
        [[nodiscard]]
        constexpr auto at(Usize const index) -> Option<T&> {
            if (index < n)
                return m_array[index];
            else
                return nullopt;
        }

        template <std::equality_comparable_with<T> T2> [[nodiscard]]
        constexpr auto operator==(Array<T2, n> const& other) const
            noexcept(noexcept(std::declval<T>() != std::declval<T2>())) -> bool
        {
            for (Usize i = 0; i != n; ++i) {
                if (m_array[i] != other.m_array[i])
                    return false;
            }
            return true;
        }

        constexpr auto swap(Array& other)
            noexcept(noexcept(m_array[0].swap(m_array[0]))) -> void
            requires swappable<T>
        {
            for (Usize i = 0; i != n; ++i) {
                m_array[i].swap(other.m_array[i]);
            }
        }
        
        constexpr auto fill(T element)
            noexcept(std::is_nothrow_copy_constructible_v<T>) -> void
        {
            for (Usize i = 0; i != n-1; ++i) {
                m_array[i] = element;
            }
            back() = std::move(element);
        }
    };

    template <class T>
    struct [[nodiscard]] Array<T, 0> {
        using ContainedType = T;
        using Iterator      = T*;
        using Sentinel      = Iterator;
        using ConstIterator = T const*;
        using ConstSentinel = ConstIterator;

        [[nodiscard]]
        constexpr auto is_empty() const noexcept -> std::true_type {
            return {};
        }
        [[nodiscard]]
        constexpr auto size() const noexcept -> Usize {
            return 0;
        }
        [[nodiscard]]
        constexpr auto begin() const noexcept -> T const* {
            return nullptr;
        }
        [[nodiscard]]
        constexpr auto begin() noexcept -> T* {
            return nullptr;
        }
        [[nodiscard]]
        constexpr auto end() const noexcept -> T const* {
            return nullptr;
        }
        [[nodiscard]]
        constexpr auto end() noexcept -> T* {
            return nullptr;
        }
        [[nodiscard]]
        constexpr auto operator==(Array) const noexcept -> std::true_type {
            return {};
        }
        [[nodiscard]]
        constexpr auto operator!=(Array) const noexcept -> std::false_type {
            return {};
        }
        constexpr auto swap(Array&) noexcept -> void {
            // no-op
        }
    };

    template <class Arg, class... Args>
    Array(Arg&&, Args&&...) -> Array<std::decay_t<Arg>, 1 + sizeof...(Args)>;

    template <class T, Usize n>
    constexpr auto to_array(T(&&array)[n]) -> Array<T, n> {
        return [&]<Usize... indices>(std::index_sequence<indices...>) {
            return Array<T, n> { std::move(array[indices])... };
        }(std::make_index_sequence<n> {});
    }
}
