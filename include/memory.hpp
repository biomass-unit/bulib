#pragma once

#include "utility.hpp"
#include "concepts.hpp"


namespace bu {
    template <class X>
    constexpr auto destroy(X& x)
        noexcept(std::is_nothrow_destructible_v<X>) -> void
    {
        x.~X();
    }
    template <iterator It, sentinel_for<It> Se>
    constexpr auto destroy(It begin, Se end)
        noexcept(noexcept(destroy(*begin))) -> void
    {
        for (; begin != end; ++begin) {
            destroy(*begin);
        }
    }


    template <class T>
    struct [[nodiscard]] DefaultDeleter {
        constexpr auto operator()(std::remove_extent_t<T>* const ptr) const -> void {
            std::is_array_v<T> ? delete[] ptr : delete ptr;
        }
    };

    // Could be an aggregate, but clangd erroneously complained
    template <class Pointer>
        requires std::is_pointer_v<Pointer>
    struct [[nodiscard]] FromOwning {
        Pointer pointer;
        constexpr FromOwning(Pointer const pointer) noexcept
            : pointer { pointer } {}
    };


    template <class T, class Deleter = DefaultDeleter<T>>
    class [[nodiscard]] UniquePtr {
        using Pointer = std::remove_extent_t<T>*;

        [[no_unique_address]]
        Deleter m_deleter;
        Pointer m_pointer = nullptr;
    public:
        using PointeeType = T;
        using DeleterType = Deleter;

        UniquePtr() = default;

        constexpr UniquePtr(FromOwning<Pointer> const owning)
            noexcept(std::is_nothrow_default_constructible_v<Deleter>)
            : m_pointer { owning.pointer } {}

        constexpr UniquePtr     (UniquePtr const&)               = delete;
        constexpr auto operator=(UniquePtr const&) -> UniquePtr& = delete;

        constexpr UniquePtr(UniquePtr&& other)
            noexcept(std::is_nothrow_move_constructible_v<Deleter>)
            : m_deleter { std::move(other.m_deleter) }
            , m_pointer { BU exchange(other.m_pointer, nullptr) } {}

        constexpr auto operator=(UniquePtr&& other)
            noexcept -> UniquePtr&
        {
            if (this != &other) {
                this->~UniquePtr();
                m_pointer = BU exchange(other.m_pointer, nullptr);
                if constexpr (std::is_move_assignable_v<Deleter>) {
                    m_deleter = std::move(other.m_deleter);
                }
            }
            return *this;
        }

        constexpr ~UniquePtr()
            noexcept(std::is_nothrow_invocable_v<Deleter, Pointer>)
        {
            reset();
        }

        constexpr auto reset(Pointer const new_owning_ptr)
            noexcept -> void
        {
            Pointer const old_owning_ptr = m_pointer;
            m_pointer = new_owning_ptr;

            if (old_owning_ptr)
                m_deleter(old_owning_ptr);
        }
        constexpr auto reset(std::nullptr_t = nullptr)
            noexcept -> void
        {
            if (m_pointer)
                m_deleter(BU exchange(m_pointer, nullptr));
        }

        [[nodiscard]]
        constexpr auto operator*() const noexcept -> T& {
            assert(m_pointer);
            return *m_pointer;
        }
        [[nodiscard]]
        constexpr auto operator->() const noexcept -> T* {
            assert(m_pointer);
            return m_pointer;
        }

        [[nodiscard]]
        constexpr auto release() noexcept -> Pointer {
            return BU exchange(m_pointer, nullptr);
        }
        [[nodiscard]]
        constexpr explicit operator bool() const noexcept {
            return m_pointer != nullptr;
        }
        [[nodiscard]]
        constexpr auto get() const noexcept -> Pointer {
            return m_pointer;
        }
        [[nodiscard]]
        constexpr auto operator[](Usize const index) const
            noexcept -> std::remove_extent_t<T>&
            requires std::is_unbounded_array_v<T>
        {
            return m_pointer[index];
        }

        constexpr auto swap(UniquePtr& other) noexcept -> void {
            BU swap(m_pointer, other.m_pointer);
            if constexpr (swappable<Deleter>) {
                BU swap(m_deleter, other.m_deleter);
            }
        }

        constexpr auto operator<=>(UniquePtr const& other) const
            noexcept -> std::compare_three_way_result_t<Pointer>
        {
            return m_pointer <=> other.m_pointer;
        }
        constexpr auto operator==(UniquePtr const& other) const
            noexcept -> bool
        {
            return m_pointer == other.m_pointer;
        }
    }; // class UniquePtr
    

    template <class T>
    constexpr auto operator==(UniquePtr<T> const& pointer, std::nullptr_t)
        noexcept -> bool
    {
        return pointer.get() == nullptr;
    }
    template <class T>
    constexpr auto operator==(std::nullptr_t, UniquePtr<T> const& pointer)
        noexcept -> bool
    {
        return pointer.get() == nullptr;
    }
    template <class T>
    constexpr auto operator!=(UniquePtr<T> const& pointer, std::nullptr_t)
        noexcept -> bool
    {
        return pointer.get() != nullptr;
    }
    template <class T>
    constexpr auto operator!=(std::nullptr_t, UniquePtr<T> const& pointer)
        noexcept -> bool
    {
        return pointer.get() != nullptr;
    }
    

    template <class T, class... Args>
    constexpr auto make_unique(Args&&... args) -> UniquePtr<T>
        requires std::is_constructible_v<T, Args&&...>
              && (!std::is_array_v<T>)
    {
        return FromOwning { new T(std::forward<Args>(args)...) };
    }
    template <class T>
    constexpr auto make_unique(Usize const extent) -> UniquePtr<T>
        requires std::is_unbounded_array_v<T>
    {
        return FromOwning { new std::remove_extent_t<T>[extent] {} };
    }
}
