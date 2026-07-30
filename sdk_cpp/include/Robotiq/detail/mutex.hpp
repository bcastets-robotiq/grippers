// Copyright (c) 2026 Robotiq, Inc.
//
// Licensed under the BSD-3-Clause license; see LICENSE for details.

//! \brief The lock type the SDK uses where it must serialize access.
//! Selected at compile time:
//!   1. a hosted threaded runtime  -> std::mutex;
//!   2. an RTOS BasicLockable adapter, if one is enabled (ThreadX provided — add
//!      an #elif for another RTOS);
//!   3. otherwise a no-op lock — a single-threaded (bare superloop) target needs
//!      none, and it still satisfies BasicLockable so lock sites stay one path.
//! std::lock_guard is available from <mutex> regardless; only std::mutex the type
//! depends on the runtime, so GRIPPERS_HAS_STD_THREADS selects case 1.

#pragma once

#include <mutex> // std::lock_guard (always available); std::mutex only when threaded

#include <Robotiq/detail/std_threading.hpp>

#if !GRIPPERS_HAS_STD_THREADS && defined(GRIPPERS_RTOS_THREADX)
#include <Robotiq/detail/rtos/threadx_mutex.hpp>
#endif

namespace Robotiq::detail {

#if GRIPPERS_HAS_STD_THREADS
using Mutex = std::mutex;
#elif defined(GRIPPERS_RTOS_THREADX)
using Mutex = ThreadXMutex;
#else
struct Mutex
{
   void lock() noexcept {}
   void unlock() noexcept {}
};
#endif

} // namespace Robotiq::detail
