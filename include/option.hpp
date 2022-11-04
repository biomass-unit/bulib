#pragma once

#include "utility.hpp"
#include "concepts.hpp"
#include "exception.hpp"


namespace bu::dtl {
    struct OptionSentinel {};

    template <class T>
    class [[nodiscard]] OptionIterator {
        T* m_ptr;
    public:
        constexpr OptionIterator(T* const ptr) noexcept
            : m_ptr { ptr } {}
        constexpr auto operator++() noexcept -> OptionIterator& {
            m_ptr = nullptr;
            return *this;
        }
        constexpr auto operator++(int) noexcept -> OptionIterator {
            auto copy = *this;
            ++*this;
            return copy;
        }
        constexpr auto operator==(OptionSentinel) const noexcept -> bool {
            return m_ptr == nullptr;
        }
        constexpr auto operator!=(OptionSentinel) const noexcept -> bool {
            return m_ptr != nullptr;
        }
        constexpr auto operator*() const -> T& {
            if (m_ptr)
                return *m_ptr;
            else
                throw BadIndirection {};
        }
    };

}


namespace bu {
    inline constexpr struct Nullopt {} nullopt;

    using BadOptionAccess = StatelessException<"bad option access">;


    template <class T>
    class [[nodiscard]] Option {
        union {
            T m_value;
        };
        bool m_has_value;
    public:
        using ContainedType = T;
        using Iterator      = dtl::OptionIterator<T>;
        using Sentinel      = dtl::OptionSentinel;
        using ConstIterator = dtl::OptionIterator<T const>;
        using ConstSentinel = dtl::OptionSentinel;

        constexpr Option(Nullopt = nullopt) noexcept
            : m_has_value { false } {}

        template <class... Args>
        constexpr explicit Option(InPlace, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args...>)
            : m_value { std::forward<Args>(args)... }
            , m_has_value { true } {}

        template <class U = T>
            requires std::conjunction_v<
                std::is_constructible<T, U>,
                std::negation<std::is_same<std::remove_cvref_t<U>, InPlace>>
            >
        constexpr Option(U&& u)
            noexcept(std::is_nothrow_constructible_v<T, U>)
            : m_value { std::forward<U>(u) }
            , m_has_value { true } {}

        constexpr Option(Option const& other)
            noexcept(std::is_nothrow_copy_constructible_v<T>)
        {
            m_has_value = other.m_has_value;
            if (m_has_value) {
                std::construct_at(std::addressof(m_value), other.m_value);
            }
        }
        constexpr Option(Option&& other)
            noexcept(std::is_nothrow_constructible_v<T>)
        {
            m_has_value = other.m_has_value;
            if (m_has_value) {
                std::construct_at(
                    std::addressof(m_value),
                    std::move(other.m_value)
                );
            }
        }

        constexpr ~Option()
            noexcept(std::is_nothrow_destructible_v<T>)
        {
            if (m_has_value) {
                m_value.~T();
            }
        }

        constexpr auto operator=(Option const& other)
            noexcept(std::is_nothrow_constructible_v<T>
                && std::is_nothrow_copy_assignable_v<T>) -> Option&
        {
            if (m_has_value) {
                if (other.m_has_value) { // Both have values
                    m_value = other.m_value;
                }
                else { // Only this has value
                    m_value.~T();
                    m_has_value = false;
                }
            }
            else if (other.m_has_value) { // Only other has value
                std::construct_at(
                    std::addressof(m_value),
                    other.m_value
                );
                m_has_value = true;
            }
            return *this;
        }
        constexpr auto operator=(Option&& other)
            noexcept(std::is_nothrow_move_constructible_v<T>
                && std::is_nothrow_move_assignable_v<T>) -> Option&
        {
            if (m_has_value) {
                if (other.m_has_value) { // Both have values
                    m_value = std::move(other.m_value);
                }
                else { // Only this has value
                    m_value.~T();
                    m_has_value = false;
                }
            }
            else if (other.m_has_value) { // Only other has value
                std::construct_at(
                    std::addressof(m_value),
                    std::move(other.m_value)
                );
                m_has_value = true;
            }
            return *this;
        }

        [[nodiscard]]
        constexpr auto has_value() const noexcept -> bool {
            return m_has_value;
        }
        [[nodiscard]]
        constexpr auto is_empty() const noexcept -> bool {
            return !m_has_value;
        }
        [[nodiscard]]
        constexpr operator bool() const noexcept {
            return m_has_value;
        }

        [[nodiscard]]
        constexpr auto value() const -> T const& {
            if (m_has_value)
                return m_value;
            else
                throw BadOptionAccess {};
        }
        [[nodiscard]]
        constexpr auto value() -> T& {
            return const_cast<T&>(const_cast<Option const*>(this)->value());
        }

        [[nodiscard]]
        constexpr auto begin() const noexcept -> ConstIterator {
            return m_has_value ? std::addressof(m_value) : nullptr;
        }
        [[nodiscard]]
        constexpr auto begin() noexcept -> Iterator {
            return m_has_value ? std::addressof(m_value) : nullptr;
        }
        [[nodiscard]]
        constexpr auto end() const noexcept -> ConstSentinel {
            return {};
        }
        [[nodiscard]]
        constexpr auto end() noexcept -> Sentinel {
            return {};
        }

        [[nodiscard]]
        constexpr auto size() const noexcept -> Usize {
            return static_cast<Usize>(m_has_value);
        }
    };

    template <class T>
    class [[nodiscard]] Option<T&> {
        T* m_ptr = nullptr;
    public:
        using ContainedType = T&;

        Option() = default;

        constexpr Option(Nullopt) {}

        constexpr Option(T& reference) noexcept
            : m_ptr { std::addressof(reference) } {}

        constexpr Option(InPlace, T& reference) noexcept
            : Option { reference } {}

        [[nodiscard]]
        constexpr auto has_value() const noexcept -> bool {
            return m_ptr != nullptr;
        }
        [[nodiscard]]
        constexpr auto is_empty() const noexcept -> bool {
            return m_ptr == nullptr;
        }
        [[nodiscard]]
        constexpr operator bool() const noexcept {
            return has_value();
        }

        [[nodiscard]]
        constexpr auto value() const -> T& {
            if (m_ptr)
                return *m_ptr;
            else
                throw BadOptionAccess {};
        }

        [[nodiscard]]
        constexpr auto size() const noexcept -> Usize {
            return m_ptr ? 1 : 0;
        }

        constexpr auto swap(Option& other) noexcept -> void {
            swap(m_ptr, other.m_ptr);
        }
    };
}
