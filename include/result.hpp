#pragma once

#include "utility.hpp"
#include "concepts.hpp"
#include "option.hpp"
#include "exception.hpp"


namespace bu {
    using BadResultAccess = StatelessException<"bad result access">;

    class [[nodiscard]] BadResultExpectAccess : public BadResultAccess {
        char const* m_message;
    public:
        constexpr BadResultExpectAccess(char const* const msg) noexcept
            : m_message { msg } {}
        constexpr auto message() const noexcept -> char const* override {
            return m_message;
        }
    };


    namespace dtl {
        struct ResultDefaultConstructTag {};
    }

    template <class T>
    struct [[nodiscard]] Ok {
        T value;
        template <class... Args>
        constexpr Ok(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
            : value { std::forward<Args>(args)... } {}
    };
    template <class T>
    Ok(T) -> Ok<T>;
    Ok() -> Ok<dtl::ResultDefaultConstructTag>;

    template <class T>
    struct [[nodiscard]] Err {
        T value;
        template <class... Args>
        constexpr Err(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
            : value { std::forward<Args>(args)... } {}
    };
    template <class T>
    Err(T) -> Err<T>;
    Err() -> Err<dtl::ResultDefaultConstructTag>;


    template <class Good, class Bad>
    class [[nodiscard]] Result {
        union {
            Good m_good;
            Bad  m_bad;
        };
        bool m_is_good;

        using Alternatives = Typelist<Good, Bad>;
    public:
        using ContainedType = Good;
        using ErrorType     = Bad;
        using Iterator      = dtl::OptionIterator<Good>;
        using Sentinel      = dtl::OptionSentinel;
        using ConstIterator = dtl::OptionIterator<Good const>;
        using ConstSentinel = Sentinel;

        constexpr Result()
            noexcept(std::is_nothrow_default_constructible_v<Bad>)
            requires std::is_default_constructible_v<Bad>
            : m_bad {}
            , m_is_good { false } {}

        constexpr Result(Ok<dtl::ResultDefaultConstructTag>)
            noexcept(std::is_nothrow_default_constructible_v<Good>)
            requires std::is_default_constructible_v<Good>
            : m_good {}
            , m_is_good { true } {}

        constexpr Result(Err<dtl::ResultDefaultConstructTag>)
            noexcept(std::is_nothrow_default_constructible_v<Bad>)
            requires std::is_default_constructible_v<Bad>
            : m_bad {}
            , m_is_good { false } {}

        constexpr Result(Ok<Good>&& ok)
            noexcept(std::is_nothrow_move_constructible_v<Good>)
            requires std::is_move_constructible_v<Good>
            : m_good { std::move(ok.value) }
            , m_is_good { true } {}

        constexpr Result(Err<Bad>&& err)
            noexcept(std::is_nothrow_move_constructible_v<Bad>)
            requires std::is_move_constructible_v<Bad>
            : m_bad { std::move(err.value) }
            , m_is_good { false } {}

        constexpr Result(Result const& other)
            noexcept(Alternatives::template all<std::is_nothrow_copy_constructible>)
            requires Alternatives::template all<std::is_copy_constructible>
            : m_is_good { other.m_is_good }
        {
            if (m_is_good)
                std::construct_at(std::addressof(m_good), other.m_good);
            else
                std::construct_at(std::addressof(m_bad), other.m_bad);
        }

        constexpr Result(Result&& other)
            noexcept(Alternatives::template all<std::is_nothrow_move_constructible>)
            requires Alternatives::template all<std::is_move_constructible>
            : m_is_good { other.m_is_good }
        {
            if (m_is_good)
                std::construct_at(std::addressof(m_good), std::move(other.m_good));
            else
                std::construct_at(std::addressof(m_bad), std::move(other.m_bad));
        }

        constexpr auto operator=(Result const& other)
            noexcept(Alternatives::template all<std::is_nothrow_copy_assignable>
                  && Alternatives::template all<std::is_nothrow_copy_constructible>) -> Result&
            requires Alternatives::template all<std::is_copy_assignable>
                  && Alternatives::template all<std::is_copy_constructible>
        {
            if (this != &other) {
                if (m_is_good == other.m_is_good) {
                    if (m_is_good)
                        m_good = other.m_good;
                    else
                        m_bad = other.m_bad;
                }
                else if (m_is_good) { // this.is_ok && other.is_err
                    m_good.~Good();
                    std::construct_at(std::addressof(m_bad), other.m_bad);
                    m_is_good = false;
                }
                else { // this.is_err && other.is_ok
                    m_bad.~Bad();
                    std::construct_at(std::addressof(m_good), other.m_good);
                    m_is_good = true;
                }
            }
            return *this;
        }

        constexpr auto operator=(Result&& other)
            noexcept(Alternatives::template all<std::is_nothrow_move_assignable>
                  && Alternatives::template all<std::is_nothrow_move_constructible>) -> Result&
            requires Alternatives::template all<std::is_move_assignable>
                  && Alternatives::template all<std::is_move_constructible>
        {
            if (this != &other) {
                if (m_is_good == other.m_is_good) {
                    if (m_is_good)
                        m_good = std::move(other.m_good);
                    else
                        m_bad = std::move(other.m_bad);
                }
                else if (m_is_good) { // this.is_ok && other.is_err
                    m_good.~Good();
                    std::construct_at(std::addressof(m_bad), std::move(other.m_bad));
                    m_is_good = false;
                }
                else { // this.is_err && other.is_ok
                    m_bad.~Bad();
                    std::construct_at(std::addressof(m_good), std::move(other.m_good));
                    m_is_good = true;
                }
            }
            return *this;
        }

        constexpr ~Result()
            noexcept(Alternatives::template all<std::is_nothrow_destructible>)
        {
            m_is_good ? m_good.~Good() : m_bad.~Bad();
        }

        [[nodiscard]]
        constexpr auto value() const -> Good const& {
            if (m_is_good)
                return m_good;
            else
                throw BadResultAccess {};
        }
        [[nodiscard]]
        constexpr auto value() -> Good& {
            return const_cast<Good&>(const_cast<Result const*>(this)->value());
        }

        [[nodiscard]]
        constexpr auto error() const -> Bad const& {
            if (m_is_good)
                throw BadResultAccess {};
            else
                return m_bad;
        }
        [[nodiscard]]
        constexpr auto error() -> Bad& {
            return const_cast<Bad&>(const_cast<Result const*>(this)->error());
        }

        [[nodiscard]]
        constexpr auto expect(char const* const message) const -> Good const& {
            if (m_is_good)
                return m_good;
            else
                throw BadResultExpectAccess { message };
        }
        [[nodiscard]]
        constexpr auto expect(char const* const message) -> Good& {
            return const_cast<Good&>(const_cast<Result const*>(this)->expect(message));
        }

        [[nodiscard]]
        constexpr auto expect_err(char const* const message) const -> Bad const& {
            if (m_is_good)
                throw BadResultExpectAccess { message };
            else
                return m_bad;
        }
        [[nodiscard]]
        constexpr auto expect_err(char const* const message) -> Bad& {
            return const_cast<Bad&>(const_cast<Result const*>(this)->expect_err(message));
        }

        template <class Arg> [[nodiscard]]
        constexpr auto value_or(Arg&& arg) const&
            noexcept(std::is_nothrow_copy_constructible_v<Good>
                  && std::is_nothrow_constructible_v<Good, Arg&&>) -> Good
            requires std::is_copy_constructible_v<Good>
                  && std::is_constructible_v<Good, Arg&&>
        {
            if (m_is_good)
                return m_good;
            else
                return static_cast<Good>(std::forward<Arg>(arg));
        }
        template <class Arg> [[nodiscard]]
        constexpr auto value_or(Arg&& arg) &&
            noexcept(std::is_nothrow_move_constructible_v<Good>
                  && std::is_nothrow_constructible_v<Good, Arg&&>) -> Good
            requires std::is_move_constructible_v<Good>
                  && std::is_constructible_v<Good, Arg&&>
        {
            if (m_is_good)
                return std::move(m_good);
            else
                return static_cast<Good>(std::forward<Arg>(arg));
        }

        template <class Arg> [[nodiscard]]
        constexpr auto error_or(Arg&& arg) const&
            noexcept(std::is_nothrow_copy_constructible_v<Bad>
                  && std::is_nothrow_constructible_v<Bad, Arg&&>) -> Bad
            requires std::is_copy_constructible_v<Bad>
                  && std::is_constructible_v<Bad, Arg&&>
        {
            if (m_is_good)
                return static_cast<Bad>(std::forward<Arg>(arg));
            else
                return m_bad;
        }
        template <class Arg> [[nodiscard]]
        constexpr auto value_or(Arg&& arg) &&
            noexcept(std::is_nothrow_move_constructible_v<Bad>
                  && std::is_nothrow_constructible_v<Bad, Arg&&>) -> Bad
            requires std::is_move_constructible_v<Bad>
                  && std::is_constructible_v<Bad, Arg&&>
        {
            if (m_is_good)
                return static_cast<Bad>(std::forward<Arg>(arg));
            else
                return std::move(m_bad);
        }

        [[nodiscard]]
        constexpr auto begin() const noexcept -> ConstIterator {
            return m_is_good ? std::addressof(m_good) : nullptr;
        }
        [[nodiscard]]
        constexpr auto begin() noexcept -> Iterator {
            return m_is_good ? std::addressof(m_good) : nullptr;
        }
        [[nodiscard]]
        constexpr auto end() const noexcept -> ConstSentinel {
            return {};
        }

        [[nodiscard]]
        constexpr auto is_ok() const noexcept -> bool {
            return m_is_good;
        }
        [[nodiscard]]
        constexpr auto is_err() const noexcept -> bool {
            return !m_is_good;
        }

        [[nodiscard]]
        constexpr auto size() const noexcept -> Usize {
            return static_cast<Usize>(m_is_good);
        }
        [[nodiscard]]
        constexpr auto is_empty() const noexcept -> bool {
            return !m_is_good;
        }
        [[nodiscard]]
        constexpr explicit operator bool() const noexcept {
            return m_is_good;
        }

        [[nodiscard]]
        constexpr auto operator==(Result const& other) const
            noexcept(noexcept(m_good == m_good, m_bad == m_bad)) -> bool
        {
            if (m_is_good != other.m_is_good)
                return false;
            return m_is_good
                ? m_good == other.m_good
                : m_bad  == other.m_bad;
        }
    };
}
