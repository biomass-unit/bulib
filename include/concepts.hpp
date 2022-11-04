#pragma once


namespace bu {
    template <class X>
    concept incrementable = requires (X x) {
        { ++x } -> std::same_as<X&>;
        { x++ } -> std::same_as<X>;
    };
    template <class X>
    concept decrementable = requires (X x) {
        { --x } -> std::same_as<X&>;
        { x-- } -> std::same_as<X>;
    };

    template <class It>
    concept iterator =
        incrementable<It> && requires (It it) { *it; };

    template <class It>
    concept bidirectional_iterator =
        iterator<It> && decrementable<It>;

    template <class Se, class It>
    concept sentinel_for = requires (It it, Se se) {
        { it != se } -> std::convertible_to<bool>;
    };

    template <class X>
    concept swappable = requires (X& x) {
        ::bu::swap(x, x);
    };

    template <class C>
    concept container = requires (C c, C const cc) {
        typename C::ContainedType;
        typename C::Iterator;
        typename C::Sentinel;
        typename C::ConstIterator;
        typename C::ConstSentinel;

        requires iterator<typename C::Iterator>;
        requires iterator<typename C::ConstIterator>;
        requires sentinel_for<typename C::Sentinel, typename C::Iterator>;
        requires sentinel_for<typename C::ConstSentinel, typename C::ConstIterator>;

        requires std::is_default_constructible_v<C>;
        requires std::is_copy_constructible_v<C>;
        requires std::is_move_constructible_v<C>;
        requires std::is_copy_assignable_v<C>;
        requires std::is_move_assignable_v<C>;
        requires std::is_destructible_v<C>;

        requires (std::equality_comparable<typename C::ContainedType>
            ? std::equality_comparable<C> : true);

        { c.begin()  } -> std::same_as<typename C::Iterator>;
        { c.end()    } -> std::same_as<typename C::Sentinel>;
        { cc.begin() } -> std::same_as<typename C::ConstIterator>;
        { cc.end()   } -> std::same_as<typename C::ConstSentinel>;

        { cc.size()     } -> std::same_as<Usize>;
        { cc.is_empty() } -> std::convertible_to<bool>;
    };
}
