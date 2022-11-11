#pragma once

#include "option.hpp"
#include "utility.hpp"
#include "concepts.hpp"
#include "exception.hpp"


namespace bu {
    template <class T, Usize extent>
    struct [[nodiscard]] Array {
        T m_array[extent];

        using ContainedType = T;
        using SizeType      = Usize;
        using Iterator      = T*;
        using Sentinel      = Iterator;
        using ConstIterator = T const*;
        using ConstSentinel = ConstIterator;
    
        [[nodiscard]]
        constexpr auto is_empty() const noexcept -> bool {
            return false;
        }
        [[nodiscard]]
        constexpr auto size() const noexcept -> SizeType {
            return extent;
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
            return m_array[extent - 1];
        }
        [[nodiscard]]
        constexpr auto back() noexcept -> T& {
            return m_array[extent - 1];
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
            return m_array + extent;
        }
        [[nodiscard]]
        constexpr auto end() noexcept -> T* {
            return m_array + extent;
        }

        [[nodiscard]]
        constexpr auto operator[](SizeType const index) const -> T const& {
            if (index < extent)
                return m_array[index];
            else
                throw OutOfRange {};
        }
        [[nodiscard]]
        constexpr auto operator[](SizeType const index) -> T& {
            return const_cast<T&>(const_cast<Array const&>(*this)[index]);
        }

        [[nodiscard]]
        constexpr auto at(SizeType const index) const -> Option<T const&> {
            if (index < extent)
                return m_array[index];
            else
                return nullopt;
        }
        [[nodiscard]]
        constexpr auto at(SizeType const index) -> Option<T&> {
            if (index < extent)
                return m_array[index];
            else
                return nullopt;
        }

        template <std::equality_comparable_with<T> T2> [[nodiscard]]
        constexpr auto operator==(Array<T2, extent> const& other) const
            noexcept(noexcept(std::declval<T>() != std::declval<T2>())) -> bool
        {
            for (SizeType i = 0; i != extent; ++i) {
                if (m_array[i] != other.m_array[i])
                    return false;
            }
            return true;
        }

        constexpr auto swap(Array& other)
            noexcept(noexcept(BU swap(m_array[0], m_array[0]))) -> void
            requires swappable<T>
        {
            for (SizeType i = 0; i != extent; ++i) {
                BU swap(m_array[i], other.m_array[i]);
            }
        }
        
        constexpr auto fill(T element)
            noexcept(std::is_nothrow_copy_constructible_v<T>) -> void
        {
            for (SizeType i = 0; i != extent-1; ++i) {
                m_array[i] = element;
            }
            back() = std::move(element);
        }
    };

    template <class T>
    struct [[nodiscard]] Array<T, 0> {
        using ContainedType = T;
        using SizeType      = Usize;
        using Iterator      = T*;
        using Sentinel      = Iterator;
        using ConstIterator = T const*;
        using ConstSentinel = ConstIterator;

        [[nodiscard]]
        constexpr auto is_empty() const noexcept -> bool {
            return true;
        }
        [[nodiscard]]
        constexpr auto size() const noexcept -> SizeType {
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
        constexpr auto operator==(Array) const noexcept -> bool {
            return true;
        }
        [[nodiscard]]
        constexpr auto operator!=(Array) const noexcept -> bool {
            return false;
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
