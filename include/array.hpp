#pragma once

#include "option.hpp"
#include "utility.hpp"
#include "exception.hpp"


namespace bu {
    template <class T, Usize n>
    struct [[nodiscard]] Array {
        T m_array[n];
    
        [[nodiscard]]
        constexpr auto is_empty() const noexcept -> bool {
            return false;
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

        constexpr auto operator[](Usize const index) const -> T const& {
            if (index < n)
                return m_array[index];
            else
                throw OutOfRange {};
        }
        constexpr auto operator[](Usize const index) -> T& {
            return const_cast<T&>(const_cast<Array const&>(*this)[index]);
        }

        constexpr auto at(Usize const index) const -> Option<T const&> {
            if (index < n)
                return m_array[index];
            else
                return nullopt;
        }
        constexpr auto at(Usize const index) -> Option<T&> {
            if (index < n)
                return m_array[index];
            else
                return nullopt;
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
        [[nodiscard]]
        constexpr auto is_empty() const noexcept -> bool {
            return true;
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
