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

    class Exception {
    public:
        virtual auto message() const noexcept -> char const* = 0;
    };

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

}
