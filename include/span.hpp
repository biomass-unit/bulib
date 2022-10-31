#pragma once

#include "utility.hpp"
#include "array.hpp"


namespace bu {
    struct BadSlice : Exception {
        auto message() const noexcept -> char const* override {
            return "bad slice operation";
        }
    };

    template <class T>
    class [[nodiscard]] Span {
        T*    m_ptr = nullptr;
        Usize m_len = 0;
    public:
        Span() = default;

        constexpr explicit Span(T* const start, Usize const length) noexcept
            : m_ptr { start }
            , m_len { length } {}

        constexpr explicit Span(T* const start, T* const stop) noexcept
            : Span { start, unsigned_distance(start, stop) } {}

        template <Usize n>
        constexpr Span(T(&array)[n]) noexcept
            : m_ptr { array }
            , m_len { n } {}

        template <Usize n>
        constexpr Span(Array<T, n>& array) noexcept
            : m_ptr { array.data() }
            , m_len { n } {}

        constexpr auto remove_prefix(Usize const off) -> void {
            if (m_len < off) {
                throw BadSlice {};
            }
            m_ptr += off;
            m_len -= off;
        }
        constexpr auto without_prefix(Usize const off) const -> Span {
            Span copy = *this;
            copy.remove_prefix(off);
            return copy;
        }
        constexpr auto remove_suffix(Usize const off) -> void {
            if (m_len < off) {
                throw BadSlice {};
            }
            m_len -= off;
        }
        constexpr auto without_suffix(Usize const off) const -> Span {
            Span copy = *this;
            copy.remove_suffix(off);
            return copy;
        }

        constexpr auto begin() const noexcept -> T* {
            return m_ptr;
        }
        constexpr auto end() const noexcept -> T* {
            return m_ptr + m_len;
        }
    };
}
