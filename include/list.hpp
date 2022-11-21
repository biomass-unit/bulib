#pragma once

#include "utility.hpp"
#include "concepts.hpp"
#include "allocator.hpp"
#include "exception.hpp"


namespace bu {
    template <class T>
    struct [[nodiscard]] ListNode {
        [[no_unique_address]]
        T         value;
        ListNode* next = nullptr;
        ListNode* prev = nullptr;

        template <class... Args>
        constexpr ListNode(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
            : value(std::forward<Args>(args)...) {}
    };

    namespace dtl {
        template <class T, bool is_const>
        class ListIterator {
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
            [[nodiscard]]
            constexpr auto operator--() -> ListIterator& {
                if (m_node) {
                    m_node = m_node->prev;
                    return *this;
                }
                else throw BadIndirection {};
            }
            [[nodiscard]]
            constexpr auto operator--(int) -> ListIterator {
                auto copy = *this;
                --*this;
                return copy;
            }
            [[nodiscard]]
            constexpr auto operator*() const
                -> std::conditional_t<is_const, T const, T>&
            {
                if (m_node)
                    return m_node->value;
                else
                    throw BadIndirection {};
            }

            [[nodiscard]]
            constexpr auto operator==(ListIterator const&) const
                noexcept -> bool = default;

            [[nodiscard]]
            constexpr auto is_end_iterator() const
                noexcept -> bool
            {
                return m_node == nullptr;
            }

            // Enable conversion of non-const to const iterators
            [[nodiscard]]
            constexpr operator ListIterator<T, true>() const
                noexcept requires (!is_const)
            {
                return ListIterator<T, true> { m_node };
            }

            [[nodiscard]]
            constexpr auto get_node() const noexcept -> ListNode<T>* {
                return m_node;
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
        using Sentinel      = Iterator;
        using ConstIterator = dtl::ListIterator<T, true>;
        using ConstSentinel = ConstIterator;

        List() = default;

        constexpr List(Usize count, T element)
            noexcept(std::is_nothrow_default_constructible_v<A>
                && std::is_nothrow_copy_constructible_v<T>
                && std::is_nothrow_move_constructible_v<T>)
        {
            if (!count)
                return;
            while (--count) {
                append(element);
            }
            append(std::move(element));
        }

        template <Usize n>
        constexpr List(T(&&initializers)[n], A allocator = A {})
            noexcept(std::is_nothrow_move_constructible_v<T>)
            : m_allocator { std::move(allocator) }
        {
            for (T& initializer : initializers) {
                append(std::move(initializer));
            }
        }

        constexpr List(List const& other)
            noexcept(Typelist<T, A>::template all<std::is_nothrow_copy_constructible>
                && nothrow_alloc<A>)
            : m_allocator { other.m_allocator }
        {
            for (T const& element : other) {
                append(element);
            }
        }

        constexpr List(List&& other) noexcept
            : m_allocator { std::move(other.m_allocator) }
            , m_head { BU exchange(other.m_head, nullptr) }
            , m_tail { BU exchange(other.m_tail, nullptr) }
            , m_len  { BU exchange(other.m_len, 0) } {}

        constexpr auto operator=(List const& other)
            noexcept(true) -> List&
        {
            if (this == &other)
                return *this;

            if constexpr (bu::AllocatorTraits<A>::propagate_on_copy_assign) {
                m_allocator = other.m_allocator;
            }

            Iterator      a =       begin();
            ConstIterator b = other.begin();

            if (m_len == other.m_len) { // Member-wise assignment
                while (!a.is_end_iterator()) {
                    *a++ = *b++;
                }
                return *this;
            }
            else if (m_len < other.m_len) {
                while (!a.is_end_iterator()) {
                    *a++ = *b++;
                }
                while (!b.is_end_iterator()) {
                    append(*b++);
                }
                return *this;
            }
            else { // m_len > other.m_len
                while (!b.is_end_iterator()) {
                    *a++ = *b++;
                }
                while (m_len > other.m_len) {
                    erase(Iterator { m_tail });
                }
                return *this;
            }
        }

        constexpr auto operator=(List&& other)
            noexcept -> List&
        {
            this->~List();
            return *std::construct_at(this, std::move(other));
        }

        constexpr ~List() noexcept(noexcept(clear())) {
            clear();
        }

        constexpr auto clear()
            noexcept(std::is_nothrow_destructible_v<T>
                && nothrow_dealloc<T>) -> void
        {
            Node* node = m_head;
            while (node) {
                Node* const next_node = node->next;
                delete_node(node);
                node = next_node;
            }
            m_head = nullptr;
            m_tail = nullptr;
            m_len  = 0;
        }

        /* Description:
         *     Creates a new node with a value constructed by
         *     `T(std::forward<Args>(args)...)` and inserts it
         *     before `where`. If `where` is the end iterator,
         *     equivalent to append. If `where` is the begin
         *     iterator, equivalent to prepend.
         *
         * Return value:
         *     Iterator to the newly inserted node.
         *
         * Exceptions:
         *     Invokes potentially throwing operations:
         *     - T::T(Args&&...)
         *     - A::allocate(bu::Usize)
         *
         * Preconditions:
         *     `where` must be an iterator into `this`.
         */
        template <class... Args>
        constexpr auto insert(ConstIterator const where, Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>
                && nothrow_alloc<A>) -> Iterator
        {
            Node* const new_node  = make_node(std::forward<Args>(args)...);
            Node* const successor = where.get_node();

            if (successor) {
                if (successor == m_head) { // Prepend
                    if (m_head) {
                        new_node->next = m_head;
                        m_head->prev = new_node;
                        m_head = new_node;
                    }
                    else {
                        m_head = m_tail = new_node;
                    }
                }
                else { // Middle
                    assert(m_len >= 2);

                    new_node->prev = successor->prev;
                    new_node->next = successor;

                    successor->prev->next = new_node;
                    successor->prev = new_node;
                }
            }
            else { // Append
                if (m_tail) {
                    new_node->prev = m_tail;
                    m_tail->next = new_node;
                    m_tail = new_node;
                }
                else {
                    m_head = m_tail = new_node;
                }
            }

            ++m_len;
            return Iterator { new_node };
        }

        template <class... Args>
        constexpr auto append(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>
                && nothrow_alloc<A>) -> void
        {
            (void)insert(end(), std::forward<Args>(args)...);
        }

        template <class... Args>
        constexpr auto prepend(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>
                && nothrow_alloc<A>) -> void
        {
            (void)insert(begin(), std::forward<Args>(args)...);
        }

        /* Description:
         *     Erases the node at `where`.
         *
         * Return value:
         *     Iterator to the node that comes after `where`, or
         *     the end iterator if there is no node after `where`.
         *
         * Exceptions:
         *     Invokes potentially throwing operations:
         *     - A::deallocate(T*, bu::Usize)
         *     - T::~T()
         *
         * Preconditions:
         *     `where` must be an iterator into `this`.
         */
        constexpr auto erase(ConstIterator const where)
            noexcept(std::is_nothrow_destructible_v<T>
                && nothrow_dealloc<A>) -> Iterator
        {
            Node* const node = where.get_node();
            if (!node) {
                return Iterator { nullptr };
            }

            Node* const prev = node->prev;
            Node* const next = node->next;

            delete_node(node);

            if (prev) {
                prev->next = next;
            }
            else {
                m_head = next;
            }

            if (next) {
                next->prev = prev;
            }
            else {
                m_tail = prev;
            }

            --m_len;
            return Iterator { next };
        }

        template <std::predicate<T const&, T const&> BinaryPredicate>
        constexpr auto unique(BinaryPredicate predicate)
            noexcept(std::is_nothrow_invocable_v<BinaryPredicate, T const&, T const&>
                && nothrow_dealloc<A>) -> Usize
        {
            // If the list contains fewer than 2 elements no duplicates can exist
            if (m_len < 2) return 0;

            Usize    erased_count = 0;
            Iterator previous     = begin();

            for (Iterator it = ++begin(); it != Sentinel {};) {
                assert(it.get_node() != nullptr);
                if (std::invoke(predicate, *it, *previous)) {
                    it = erase(it);
                    ++erased_count;
                }
                else {
                    previous = it++;
                }
            }

            return erased_count;
        }

        constexpr auto unique()
            noexcept(noexcept(unique(std::equal_to<T>{}))) -> Usize
            requires std::equality_comparable<T>
        {
            return unique(std::equal_to<T>{});
        }

        constexpr auto begin() const noexcept -> ConstIterator {
            return ConstIterator { m_head };
        }
        constexpr auto begin() noexcept -> Iterator {
            return Iterator { m_head };
        }
        constexpr auto end() const noexcept -> ConstSentinel {
            return ConstSentinel { nullptr };
        }
        constexpr auto end() noexcept -> Sentinel {
            return Sentinel { nullptr };
        }

        constexpr auto size() const noexcept -> Usize {
            return m_len;
        }
        constexpr auto is_empty() const noexcept -> bool {
            return m_len == 0;
        }

        template <std::equality_comparable_with<T> T2> [[nodiscard]]
        constexpr auto operator==(List<T2> const& other) const
            noexcept(noexcept(std::declval<T const&>() == std::declval<T2 const&>())) -> bool
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
        template <class... Args>
        constexpr auto make_node(Args&&... args)
            noexcept(std::is_nothrow_constructible_v<T, Args&&...>
                && nothrow_alloc<A>) -> Node*
        {
            Node* const node = m_allocator.allocate(1);
            try {
                return std::construct_at(node, std::forward<Args>(args)...);
            }
            catch (...) {
                m_allocator.deallocate(node, 1);
                throw;
            }
        }
        constexpr auto delete_node(Node* const node)
            noexcept(std::is_nothrow_destructible_v<T>
                && nothrow_dealloc<A>) -> void
        {
            node->~Node();
            m_allocator.deallocate(node, 1);
        }
    }; // cass List
} // namespace bu
