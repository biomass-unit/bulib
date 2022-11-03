#pragma once

#include "utility.hpp"
#include "exception.hpp"


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

        template <class U>
        constexpr Option(Option<U> const& other)
            noexcept(std::is_nothrow_constructible_v<T, U const&>)
        {
            m_has_value = other.m_has_value;
            if (m_has_value) {
                std::construct_at(std::addressof(m_value), other.m_value);
            }
        }
        template <class U>
        constexpr Option(Option<U>&& other)
            noexcept(std::is_nothrow_constructible_v<T, U&&>)
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

        template <class U>
        constexpr auto operator=(Option<U> const& other)
            noexcept(std::is_nothrow_constructible_v<T, U const&>
                && std::is_nothrow_assignable_v<T, U const&>)
            -> Option&
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
        template <class U>
        constexpr auto operator=(Option<U>&& other)
            noexcept(std::is_nothrow_constructible_v<T, U&&>
                && std::is_nothrow_assignable_v<T, U&&>)
            -> Option&
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
    };

    template <class T>
    class [[nodiscard]] Option<T&> {
        T* m_ptr = nullptr;
    public:
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
    };
}
