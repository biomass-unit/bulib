#pragma once

#include "utility.hpp"
#include "exception.hpp"
#include "allocator.hpp"


namespace bu {
    template <class T, allocator_for<T> A = DefaultAllocator<T>>
    class [[nodiscard]] Vector {
        [[no_unique_address]]
        A     m_allocator;
        T*    m_ptr = nullptr;
        Usize m_len = 0;
        Usize m_cap = 0;
    public:
        using ContainedType = T;
        using AllocatorType = A;
        using Iterator      = T*;
        using ConstIterator = T const*;

        Vector() = default;

        constexpr Vector(Usize const count)
            noexcept(std::is_nothrow_default_constructible_v<T> &&
                     std::is_nothrow_default_constructible_v<A>)
        {
            if (!count)
                return;
            m_ptr = allocate(count);
            for (T* ptr = m_ptr; ptr != m_ptr + m_len; ++ptr) {
                std::construct_at(ptr);
            }
        }

        constexpr ~Vector()
            noexcept(std::is_nothrow_destructible_v<T> && nothrow_dealloc<A>)
        {
            for (T* ptr = m_ptr; ptr != m_ptr + m_len; ++ptr) {
                std::destroy_at(ptr);
            }
            deallocate(m_ptr, m_cap);
        }

        [[nodiscard]] constexpr auto data() const noexcept -> T const* { return m_ptr; }
        [[nodiscard]] constexpr auto data()       noexcept -> T      * { return m_ptr; }
    private:
        constexpr auto allocate(Usize const count)
            noexcept(nothrow_alloc<A>) -> T*
        {
            return m_allocator.allocate(count);
        }
        constexpr auto deallocate(T* ptr, Usize const count)
            noexcept(nothrow_dealloc<A>) -> void
        {
            m_allocator.deallocate(ptr, count);
        }
    };
}
