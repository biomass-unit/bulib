#pragma once

#include <new>

#include "utility.hpp"


namespace bu {
    template <class A>
    concept allocator = requires (A a, Usize const count) {
        typename A::AllocatedType;
        { a.allocate(count) } -> std::same_as<typename A::AllocatedType*>;
        a.deallocate(static_cast<typename A::AllocatedType*>(nullptr), count);
    };

    template <class A, class T>
    concept allocator_for = allocator<A> && std::same_as<typename A::AllocatedType, T>;

    template <class A>
    concept nothrow_alloc = requires (A a, Usize const count) {
        { a.allocate(count) } noexcept;
    };
    template <class A>
    concept nothrow_dealloc = requires (A a, Usize const count) {
        { a.deallocate(static_cast<typename A::AllocatedType*>(nullptr), count) } noexcept;
    };

    template <class T>
    class [[nodiscard]] DefaultAllocator {
        static constexpr auto alignment = static_cast<std::align_val_t>(alignof(T));
    public:
        using AllocatedType = T;

        static constexpr auto allocate(Usize const count) -> T* {
            return static_cast<T*>(::operator new(sizeof(T) * count, alignment));
        }
        static constexpr auto deallocate(T* const ptr, [[maybe_unused]] Usize const count) -> void {
            ::operator delete(ptr, alignment);
        }
    };

    template <class A>
    struct AllocatorTraits {
        AllocatorTraits() = delete;

        static constexpr bool propagate_on_copy_assign = false;
        static constexpr bool propagate_on_move_assign = false;
        static constexpr bool propagate_on_swap        = false;
    };
}
