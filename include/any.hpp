#pragma once

#include "utility.hpp"
#include "exception.hpp"


namespace bu {
    using BadAnyCast = StatelessException<"bad any cast">;
}

namespace bu::dtl {
    enum class [[nodiscard]] AnyState {
        trivial_big,
        trivial_small,
        nontrivial_big,
        nontrivial_small,
    };

    struct EmptyAnyVtable {};

    struct MoveAnyVtable {
    };
    struct CopyAnyVtable {
    };
    template <bool is_movable, bool is_copyable>
    struct AnyVtable {
        void (*destructor)      (void      *)                noexcept = nullptr;
        void (*move_constructor)(void      * from, void* to) noexcept = nullptr;
        void (*move_assignment) (void      * from, void* to) noexcept = nullptr;
        void (*copy_constructor)(void const* from, void* to)          = nullptr;
        void (*copy_assignment) (void const* from, void* to)          = nullptr;

        std::type_info const* rtti;
        Usize                 type_size;
        bool                  type_is_small;
        AnyState              state;
    };


    inline constexpr Usize small_any_buffer_size = 40;

    template <class T>
    constexpr bool fits_in_small_any_buffer =
        sizeof(T) <= small_any_buffer_size;

    template <class T, bool is_movable, bool is_copyable>
    constexpr auto vtable_for = [] {
        static_assert(alignof(T) <= alignof(std::max_align_t),
            "This assertion should never be reached, it is a mere failsafe");

        auto table = AnyVtable<is_movable, is_copyable>
        {
            .destructor    = [](void* const ptr) noexcept { static_cast<T*>(ptr)->~T(); },
            .rtti          = &typeid(T),
            .type_size     = sizeof(T),
            .type_is_small = fits_in_small_any_buffer<T>,
            .state         = fits_in_small_any_buffer<T>
                ? std::is_trivial_v<T>
                    ? AnyState::trivial_small
                    : AnyState::nontrivial_small
                : std::is_trivial_v<T>
                    ? AnyState::trivial_big
                    : AnyState::nontrivial_big
        };
        if constexpr (is_movable) {
            table.move_constructor = [](void* from, void* to) noexcept {
                std::construct_at(
                    static_cast<T*>(to),
                    std::move(*static_cast<T*>(from))
                );
            };
            table.move_assignment = [](void* from, void* to) noexcept {
                *static_cast<T*>(to) = std::move(*static_cast<T*>(from));
            };
        }
        if constexpr (is_copyable) {
            table.copy_constructor = [](void const* const from, void* const to) {
                std::construct_at(
                    static_cast<T*>(to),
                    *static_cast<T const*>(from)
                );
            };
            table.copy_assignment = [](void const* const from, void* const to) {
                *static_cast<T*>(to) = *static_cast<T const*>(from);
            };
        }
        return table;
    }();

    union alignas(std::max_align_t) AnyValue {
        std::byte  small[small_any_buffer_size];
        std::byte* big;
    };

    template <bool is_movable, bool is_copyable>
    class [[nodiscard]] BasicAny {
        using Vtable = AnyVtable<is_movable, is_copyable>;
        AnyValue      m_value;
        Vtable const* m_table;
    public:
        BasicAny() noexcept
            : m_table { nullptr } {}

        template <class... Args, std::constructible_from<Args&&...> T>
        explicit BasicAny(InPlaceType<T>, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
            requires(alignof(T) <= alignof(std::max_align_t))
            : m_table { &vtable_for<T, is_movable, is_copyable> }
        {
            std::construct_at(
                make_storage_for<T>(),
                std::forward<Args>(args)...
            );
        }

        template <class Arg, class Stored = std::decay_t<Arg>>
        explicit BasicAny(Arg&& arg)
            noexcept(std::is_nothrow_constructible_v<Stored, Arg&&>)
            requires(alignof(Stored) <= alignof(std::max_align_t)
                  && !std::same_as<BasicAny, Stored>)
            : BasicAny { in_place_type<Stored>, std::forward<Arg>(arg) } {}

        BasicAny(BasicAny const& other) requires is_copyable
            : m_table { other.m_table }
        {
            if (!m_table)
                return;
            switch (m_table->state) {
            case AnyState::nontrivial_big:
            {
                m_value.big = allocate_dynamic_storage(m_table->type_size);
                m_table->copy_constructor(other.m_value.big, m_value.big);
                return;
            }
            case AnyState::nontrivial_small:
            {
                m_table->copy_constructor(other.m_value.small, m_value.small);
                return;
            }
            case AnyState::trivial_big:
            {
                m_value.big = allocate_dynamic_storage(m_table->type_size);
                std::memcpy(m_value.big, other.m_value.big, m_table->type_size);
                return;
            }
            case AnyState::trivial_small:
            {
                std::memcpy(m_value.small, other.m_value.small, m_table->type_size);
                return;
            }
            default:
                BU unreachable();
            }
        }

        BasicAny(BasicAny&& other) requires is_movable
            : m_table { other.m_table }
        {
            if (!m_table)
                return;
            switch (m_table->state) {
            case AnyState::nontrivial_big:
            case AnyState::trivial_big:
            {
                m_value.big = other.m_value.big;
                other.m_table = nullptr;
                return;
            }
            case AnyState::trivial_small:
            {
                std::memcpy(m_value.small, other.m_value.small, m_table->type_size);
                return;
            }
            case AnyState::nontrivial_small:
            {
                m_table->move_constructor(other.m_value.small, m_value.small);
                return;
            }
            default:
                BU unreachable();
            }
        }

        auto operator=(BasicAny const& other)
            -> BasicAny& requires is_copyable
        {
            do_assignment<&Vtable::copy_assignment>(other);
            return *this;
        }

        auto operator=(BasicAny&& other)
            -> BasicAny& requires is_movable
        {
            do_assignment<&Vtable::move_assignment>(other);
            return *this;
        }

        ~BasicAny() {
            reset();
        }

        auto reset() -> void {
            if (!m_table)
                return;
            switch (m_table->state) {
            case AnyState::nontrivial_big:
                m_table->destructor(reinterpret_cast<void*>(m_value.big));
                deallocate_dynamic_storage(m_value.big);
                break;
            case AnyState::nontrivial_small:
                m_table->destructor(reinterpret_cast<void*>(m_value.small));
                break;
            case AnyState::trivial_big:
                deallocate_dynamic_storage(m_value.big);
                break;
            default:
                ; // no-op
            }
            m_table = nullptr;
        }

        auto has_value() const noexcept -> bool {
            return m_table != nullptr;
        }
        auto type() const noexcept -> std::type_info const& {
            return m_table ? *m_table->rtti : typeid(void);
        }

        template <class T>
        auto cast() const -> T const& {
            if (type() == typeid(T))
                return unchecked_cast<T>();
            else
                throw BadAnyCast {};
        }
        template <class T>
        auto cast() -> T& {
            return const_cast<T&>(const_cast<BasicAny const*>(this)->cast<T>());
        }
    private:
        template <class T>
        auto unchecked_cast() const noexcept -> T const& {
            if constexpr (fits_in_small_any_buffer<T>)
                return *std::launder(reinterpret_cast<T const*>(m_value.small));
            else
                return *reinterpret_cast<T const*>(m_value.big);
        }
        template <class T>
        auto make_storage_for() -> T* {
            if constexpr (fits_in_small_any_buffer<T>) {
                return reinterpret_cast<T*>(m_value.small);
            }
            else {
                m_value.big = allocate_dynamic_storage(sizeof(T));
                return reinterpret_cast<T*>(m_value.big);
            }
        }
        static auto allocate_dynamic_storage(Usize const bytes) -> std::byte* {
            return static_cast<std::byte*>(::operator new(bytes));
        }
        static auto deallocate_dynamic_storage(std::byte* const storage) -> void {
            ::operator delete(storage);
        }

        template <auto Vtable::* assignment_operator>
        auto do_assignment(auto& other) -> void {
            if (this == &other)
                return;

            if (m_table == other.m_table) { // True assignment
                switch (m_table->state) {
                case AnyState::nontrivial_big:
                {
                    (m_table->*assignment_operator)(other.m_value.big, m_value.big);
                    return;
                }
                case AnyState::nontrivial_small:
                {
                    (m_table->*assignment_operator)(other.m_value.small, m_value.small);
                    return;
                }
                case AnyState::trivial_big:
                {
                    std::memcpy(m_value.big, other.m_value.big, m_table->type_size);
                    return;
                }
                case AnyState::trivial_small:
                {
                    std::memcpy(m_value.small, other.m_value.small, m_table->type_size);
                    return;
                }
                default:
                    BU unreachable();
                }
            }

            else if (m_table && !other.m_table) { // Clear this
                reset();
                return;
            }

            else { // Reconstruct from other
                if constexpr (is_movable &&
                    std::same_as<
                        decltype(assignment_operator),
                        decltype(&Vtable::move_assignment)
                    >)
                {
                    this->~BasicAny();
                    std::construct_at(this, std::move(other));
                }
                else if constexpr (is_copyable &&
                    std::same_as<
                        decltype(assignment_operator),
                        decltype(&Vtable::copy_assignment)
                    >)
                {
                    BasicAny backup = std::move(*this);
                    this->~BasicAny();

                    try {
                        std::construct_at(this, other);
                        return;
                    }
                    catch (...) {
                        std::construct_at(this, std::move(backup));
                        throw;
                    }
                }
                else {
                    static_assert(BU always_false<decltype(assignment_operator)>);
                }
            }
        }
    };
}

namespace bu {
    using Any         = dtl::BasicAny<true,  true >;
    using MoveOnlyAny = dtl::BasicAny<true,  false>;
    using CopyOnlyAny = dtl::BasicAny<false, true >;
    using PinnedAny   = dtl::BasicAny<false, false>;
}
