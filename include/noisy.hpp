#pragma once

#include <cstdio>


namespace bu {
#define BUNOISYIMPL(f, ...) \
f noexcept { std::puts("[NOISYLOG]: bu::Noisy::" #f); __VA_ARGS__ }

    struct [[nodiscard]] Noisy {
        BUNOISYIMPL(Noisy());
        BUNOISYIMPL(Noisy(Noisy const&));
        BUNOISYIMPL(Noisy(Noisy&&));
        BUNOISYIMPL(~Noisy());
        Noisy& BUNOISYIMPL(operator=(Noisy const&), return *this;);
        Noisy& BUNOISYIMPL(operator=(Noisy&&), return *this;);
    };

#undef BUNOISYIMPL
}
