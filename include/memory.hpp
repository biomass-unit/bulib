#pragma once

#include "utility.hpp"


namespace bu {
    template <class X>
    constexpr auto destroy(X& x)
        noexcept(std::is_nothrow_destructible_v<X>) -> void
    {
        x.~X();
    }
    template <std::input_iterator It, std::sentinel_for<It> Se>
    constexpr auto destroy(It begin, Se end)
        noexcept(noexcept(destroy(*begin))) -> void
    {
        for (; begin != end; ++begin) {
            destroy(*begin);
        }
    }
}
