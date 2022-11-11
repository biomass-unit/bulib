#pragma once

#include "utility.hpp"
#include "concepts.hpp"
#include "allocator.hpp"
#include "exception.hpp"


namespace bu {
    template <class T>
    struct [[nodiscard]] ListNode {
        T         value;
        ListNode* next = nullptr;
        ListNode* prev = nullptr;

        constexpr ListNode(T&& value)
            noexcept(std::is_nothrow_move_constructible_v<T>)
            : value { std::move(value) } {}
    };

    namespace dtl {
        struct [[nodiscard]] ListSentinel {};

        template <class T, bool is_const>
        class [[nodiscard]] ListIterator {
            ListNode<T>* m_node;
        public:
            constexpr explicit ListIterator(ListNode<T>* const node) noexcept
                : m_node { node } {}

            constexpr auto operator++() -> ListIterator& {
                if (m_node) {
                    m_node = m_node->next;
                    return *this;
                }
                else throw BadIndirection {};
            }
            constexpr auto operator++(int) -> ListIterator {
                auto copy = *this;
                ++*this;
                return copy;
            }
            constexpr auto operator--() -> ListIterator& {
                if (m_node) {
                    m_node = m_node->prev;
                    return *this;
                }
                else throw BadIndirection {};
            }
            constexpr auto operator--(int) -> ListIterator {
                auto copy = *this;
                --*this;
                return copy;
            }
            constexpr auto operator*() const
                -> std::conditional_t<is_const, T const, T>&
            {
                if (m_node)
                    return m_node->value;
                else
                    throw BadIndirection {};
            }

            constexpr auto operator==(ListSentinel) const noexcept -> bool {
                return m_node == nullptr;
            }
            constexpr auto operator!=(ListSentinel) const noexcept -> bool {
                return m_node != nullptr;
            }
        }; // struct ListIterator
    } // namespace dtl

    template <class T, allocator_for<ListNode<T>> A = DefaultAllocator<ListNode<T>>>
    class [[nodiscard]] List {
        using Node = ListNode<T>;

        [[no_unique_address]]
        A     m_allocator;
        Node* m_head = nullptr;
        Node* m_tail = nullptr;
        Usize m_len  = 0;
    public:
        using ContainedType = T;
        using AllocatorType = A;
        using SizeType      = Usize;
        using Iterator      = dtl::ListIterator<T, false>;
        using Sentinel      = dtl::ListSentinel;
        using ConstIterator = dtl::ListIterator<T, true>;
        using ConstSentinel = dtl::ListSentinel;

        List() = default;

        template <Usize n>
        constexpr List(T(&&initializers)[n], A allocator = A {})
            noexcept(std::is_nothrow_move_constructible_v<T>)
            : m_allocator { std::move(allocator) }
        {
            for (Usize i = 0; i != n; ++i) {
                append(std::move(initializers[i]));
            }
        }

        constexpr ~List()
            noexcept(std::is_nothrow_destructible_v<T>
                && nothrow_dealloc<T>)
        {
            Node* node = m_head;
            while (node) {
                node->~Node();
                Node* const old_node = node;
                node = node->next;
                deallocate_node(old_node);
            }
        }

        template <class... Args>
        constexpr auto append(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>
                 && nothrow_alloc<A>) -> void
        {
            Node* const new_tail = allocate_node();
            std::construct_at(new_tail, T { std::forward<Args>(args)... });

            if (m_tail) {
                new_tail->prev = m_tail;
                m_tail->next = new_tail;
                m_tail = new_tail;
            }
            else {
                m_head = m_tail = new_tail;
            }

            ++m_len;
        }

        template <class... Args>
        constexpr auto prepend(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>
                 && nothrow_alloc<A>) -> void
        {
            Node* const new_head = allocate_node();
            std::construct_at(new_head, T { std::forward<Args>(args)... });

            if (m_head) {
                new_head->next = m_head;
                m_head->prev = new_head;
                m_head = new_head;
            }
            else {
                m_head = m_tail = new_head;
            }

            ++m_len;
        }

        constexpr auto begin() const noexcept -> ConstIterator {
            return ConstIterator { m_head };
        }
        constexpr auto begin() noexcept -> Iterator {
            return Iterator { m_head };
        }
        constexpr auto end() const noexcept -> ConstSentinel {
            return {};
        }

        constexpr auto size() const noexcept -> Usize {
            return m_len;
        }
        constexpr auto is_empty() const noexcept -> bool {
            return m_len == 0;
        }

        template <std::equality_comparable_with<T> T2> [[nodiscard]]
        constexpr auto operator==(List<T2> const& other) const
            noexcept(noexcept(std::declval<T>() == std::declval<T2>())) -> bool
        {
            if (m_len != other.m_len)
                return false;

            auto a = begin();
            auto b = other.begin();

            while (a != Sentinel {}) {
                if (*a++ != *b++)
                    return false;
            }
            return true;
        }
    private:
        constexpr auto allocate_node()
            noexcept(nothrow_alloc<A>) -> Node*
        {
            return m_allocator.allocate(1);
        }
        constexpr auto deallocate_node(Node* const node)
            noexcept(nothrow_dealloc<A>) -> void
        {
            m_allocator.deallocate(node, 1);
        }
    }; // cass List
} // namespace bu
