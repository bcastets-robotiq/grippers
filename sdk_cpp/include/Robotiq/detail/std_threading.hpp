// Copyright (c) 2026 Robotiq, Inc.
//
// Licensed under the BSD-3-Clause license; see LICENSE for details.

//! \brief GRIPPERS_HAS_STD_THREADS — 1 when the standard library provides
//! std::thread and std::mutex, 0 on a freestanding runtime that omits them.
//!
//! There is no portable way to ask this: each standard library spells its own
//! threading switch, so the probe is per-implementation and every one of them
//! must be handled. Getting it wrong is silent rather than loud — a library
//! wrongly judged thread-less downgrades detail::Mutex to a no-op lock, so
//! unhandled implementations answer 0 only after being named here.

#pragma once

// Pull in the standard library's configuration macros without dragging in a
// whole facility. <version> is C++20; the classic stand-in predates it.
#if defined(__has_include)
#if __has_include(<version>)
#include <version>
#else
#include <ciso646>
#endif
#else
#include <ciso646>
#endif

#if defined(_GLIBCXX_HAS_GTHREADS)
// libstdc++: threading rides on gthreads, absent on bare-metal multilibs.
#define GRIPPERS_HAS_STD_THREADS 1
#elif defined(_LIBCPP_VERSION)
// libc++: _LIBCPP_HAS_NO_THREADS up to 18, _LIBCPP_HAS_THREADS (1/0) from 19.
#if defined(_LIBCPP_HAS_THREADS)
#define GRIPPERS_HAS_STD_THREADS _LIBCPP_HAS_THREADS
#elif defined(_LIBCPP_HAS_NO_THREADS)
#define GRIPPERS_HAS_STD_THREADS 0
#else
#define GRIPPERS_HAS_STD_THREADS 1
#endif
#elif defined(_MSVC_STL_VERSION)
// Microsoft's STL ships no thread-less configuration.
#define GRIPPERS_HAS_STD_THREADS 1
#else
#define GRIPPERS_HAS_STD_THREADS 0
#endif
