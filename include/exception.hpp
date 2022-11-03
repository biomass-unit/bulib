#pragma once

#include "utility.hpp"


namespace bu {
    class Exception {
    public:
        virtual auto message() const noexcept -> char const* = 0;
    };

    template <Metastring message_text>
    class [[nodiscard]] StatelessException : public Exception {
    public:
        auto message() const noexcept -> char const* override {
            return message_text.string();
        }
    };

    using OutOfRange = StatelessException<"out of range">;
}
