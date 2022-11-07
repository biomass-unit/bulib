#pragma once

#include "utility.hpp"
#include "concepts.hpp"
#include "option.hpp"
#include "exception.hpp"


namespace bu {
    using BadResultAccess = StatelessException<"bad result access">;

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


    template <class Good, class Bad>
    class [[nodiscard]] Result {
        union {
            Good m_good;
            Bad  m_bad;
        };
        bool m_is_good;
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

        constexpr Result(Result const&);
        constexpr Result(Result&&);

        constexpr auto operator=(Result const&) -> Result&;
        constexpr auto operator=(Result&&) -> Result&;

        constexpr ~Result()
            noexcept(Typelist<Good, Bad>::template
                all<std::is_nothrow_destructible>)
        {
            m_is_good ? m_good.~Good() : m_bad.~Bad();
        }

        constexpr auto begin() const noexcept -> ConstIterator {
            return m_is_good ? std::addressof(m_good) : nullptr;
        }
        constexpr auto begin() noexcept -> Iterator {
            return m_is_good ? std::addressof(m_good) : nullptr;
        }
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
        constexpr operator bool() const noexcept {
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
