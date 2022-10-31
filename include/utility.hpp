#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <type_traits>
#include <utility>
#include <limits>


namespace bu {

    using Usize = std::size_t;
    using Isize = std::make_signed_t<Usize>;

    template <class T> constexpr auto maximum = std::numeric_limits<T>::max();
    template <class T> constexpr auto minimum = std::numeric_limits<T>::min();

}
