#pragma once

#include "utility.hpp"
#include "option.hpp"
#include "exception.hpp"
#include "allocator.hpp"
#include "memory.hpp"


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
        using SizeType      = Usize;
        using Iterator      = T*;
        using Sentinel      = Iterator;
        using ConstIterator = T const*;
        using ConstSentinel = ConstIterator;

        Vector() = default;

        constexpr Vector(Usize const count)
            noexcept(Typelist<T, A>::template
                all<std::is_nothrow_default_constructible>)
            : m_len { count }
            , m_cap { count }
        {
            if (!count)
                return;
            m_ptr = allocate(count);
            for (T* ptr = m_ptr; ptr != m_ptr + m_len; ++ptr) {
                std::construct_at(ptr);
            }
        }

        constexpr Vector(Vector const& other)
            noexcept(std::is_nothrow_copy_constructible_v<T>
                && nothrow_alloc<A>)
            : m_allocator { other.m_allocator }
            , m_len { other.m_len }
            , m_cap { other.m_cap }
        {
            if (m_len) {
                m_ptr = allocate(m_len);
                for (Usize i = 0; i != m_len; ++i) {
                    std::construct_at(m_ptr + i, other.m_ptr[i]);
                }
            }
        }

        constexpr Vector(Vector&& other) noexcept
            : m_allocator { std::move(other.m_allocator) }
            , m_ptr { BU exchange(other.m_ptr, nullptr) }
            , m_len { BU exchange(other.m_len, 0) }
            , m_cap { BU exchange(other.m_cap, 0) } {}

        constexpr auto operator=(Vector const& other)
            noexcept(nothrow_copyable<T>
                && std::is_nothrow_copy_assignable_v<A>) -> Vector&
        {
            // TODO: Allocator equality
            if (m_len == other.m_len) {
                for (Usize i = 0; i != m_len; ++i) {
                    m_ptr[i] = other.m_ptr[i];
                }
            }
            else if (m_len >= other.m_len) {
                Usize i = 0;
                for (; i != m_len; ++i) {
                    m_ptr[i] = other.m_ptr[i];
                }
                for (; i != other.m_len; ++i) {
                    m_ptr[i].~T();
                }
                m_len = other.m_len;
            }
            else {
                std::terminate();
            }
            return *this;
        }

        constexpr auto operator=(Vector&&) noexcept -> Vector&;

        constexpr ~Vector()
            noexcept(std::is_nothrow_destructible_v<T>
                && nothrow_dealloc<A>)
        {
            destroy(m_ptr, m_ptr + m_len);
            deallocate(m_ptr, m_cap);
        }

        [[nodiscard]]
        constexpr auto size() const noexcept -> Usize {
            return m_len;
        }
        [[nodiscard]]
        constexpr auto is_empty() const noexcept -> bool {
            return m_len == 0;
        }

        [[nodiscard]] constexpr auto data() const noexcept -> T const* { return m_ptr; }
        [[nodiscard]] constexpr auto data()       noexcept -> T      * { return m_ptr; }

        [[nodiscard]] constexpr auto begin() const noexcept -> T const* { return m_ptr; }
        [[nodiscard]] constexpr auto begin()       noexcept -> T      * { return m_ptr; }

        [[nodiscard]] constexpr auto end() const noexcept -> T const* { return m_ptr + m_len; }
        [[nodiscard]] constexpr auto end()       noexcept -> T      * { return m_ptr + m_len; }

        [[nodiscard]]
        constexpr auto operator[](Usize const index) const -> T const& {
            if (index < m_len)
                return m_ptr[index];
            else
                throw OutOfRange {};
        }
        [[nodiscard]]
        constexpr auto operator[](Usize const index) -> T& {
            return const_cast<T&>(const_cast<Vector const&>(*this)[index]);
        }

        constexpr auto at(Usize const index) const noexcept -> Option<T const&> {
            if (index < m_len)
                return m_ptr[index];
            else
                return nullopt;
        }
        constexpr auto at(Usize const index) noexcept -> Option<T&> {
            if (index < m_len)
                return m_ptr[index];
            else
                return nullopt;
        }

        template <std::equality_comparable_with<T> T2, class A2> [[nodiscard]]
        constexpr auto operator==(Vector<T2, A2> const& other) const
            noexcept(noexcept(std::declval<T>() != std::declval<T2>())) -> bool
        {
            if (m_len != other.m_len)
                return false;

            auto* const a = m_ptr;
            auto* const b = other.m_ptr;
            auto* const c = m_ptr + m_len;

            for (; a != c; ++a, ++b) {
                if (*a != *b)
                    return false;
            }
            return true;
        }

        constexpr auto swap(Vector& other) noexcept -> void {
            if constexpr (AllocatorTraits<A>::propagate_on_swap) {
                swap(m_allocator, other.m_allocator);
            }
            swap(m_ptr, other.m_ptr);
            swap(m_len, other.m_len);
            swap(m_cap, other.m_cap);
        }
    private:
        [[nodiscard]]
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
