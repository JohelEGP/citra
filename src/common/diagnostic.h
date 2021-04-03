// Copyright 2019 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

// Adapted from
// https://github.com/ericniebler/range-v3/blob/master/include/range/v3/detail/config.hpp#L185.
#define CITRA_PRAGMA(X) _Pragma(#X)
#if !CITRA_COMP_MSVC
#define CITRA_DIAGNOSTIC_PUSH CITRA_PRAGMA(GCC diagnostic push)
#define CITRA_DIAGNOSTIC_POP CITRA_PRAGMA(GCC diagnostic pop)
#define CITRA_DIAGNOSTIC_IGNORE_PRAGMAS CITRA_PRAGMA(GCC diagnostic ignored "-Wpragmas")
#define CITRA_DIAGNOSTIC_IGNORE(X)                                                                 \
    CITRA_DIAGNOSTIC_IGNORE_PRAGMAS                                                                \
    CITRA_PRAGMA(GCC diagnostic ignored "-Wunknown-pragmas")                                       \
    CITRA_PRAGMA(GCC diagnostic ignored "-Wunknown-warning-option")                                \
    CITRA_PRAGMA(GCC diagnostic ignored X)
#define CITRA_DIAGNOSTIC_IGNORE_SWITCH CITRA_DIAGNOSTIC_IGNORE("-Wswitch")
#else
#define CITRA_DIAGNOSTIC_PUSH CITRA_PRAGMA(warning(push))
#define CITRA_DIAGNOSTIC_POP CITRA_PRAGMA(warning(pop))
#define CITRA_DIAGNOSTIC_IGNORE_PRAGMAS CITRA_PRAGMA(warning(disable : 4068))
#define CITRA_DIAGNOSTIC_IGNORE(X)                                                                 \
    CITRA_DIAGNOSTIC_IGNORE_PRAGMAS CITRA_PRAGMA(warning(disable : X))
#define CITRA_DIAGNOSTIC_IGNORE_SWITCH
#endif
