// Copyright (c) 2026 Robotiq, Inc.
//
// Licensed under the BSD-3-Clause license; see LICENSE for details.

//! \brief Optional ThreadX (Azure RTOS) BasicLockable adapter.
//! On targets without a libstdc++ threaded runtime (bare-metal arm-none-eabi has
//! no gthreads, so std::mutex is unavailable) but WITH an RTOS, the SDK still
//! needs a real lock when it is driven from multiple tasks. This wraps a ThreadX
//! mutex to satisfy the C++ BasicLockable requirements (lock()/unlock()), so it
//! drops straight into std::lock_guard.
//!
//! Enable by defining GRIPPERS_RTOS_THREADX and having ThreadX (tx_api.h) on the
//! include path. Other RTOSes: provide an analogous BasicLockable and select it
//! in logger.hpp's LoggerMutex block — this class is the reference example.
//!
//! Note: construct after the ThreadX kernel is running (tx_mutex_create requires
//! an initialized kernel), e.g. create the logger/client from within a task, not
//! at static-init time.

#pragma once

#include "tx_api.h"

namespace Robotiq::detail {

class ThreadXMutex
{
public:
   ThreadXMutex() { tx_mutex_create(&_mutex, const_cast<CHAR*>("grippers"), TX_NO_INHERIT); }
   ~ThreadXMutex() { tx_mutex_delete(&_mutex); }

   ThreadXMutex(const ThreadXMutex&) = delete;
   ThreadXMutex& operator=(const ThreadXMutex&) = delete;

   void lock() { tx_mutex_get(&_mutex, TX_WAIT_FOREVER); }
   void unlock() { tx_mutex_put(&_mutex); }

private:
   TX_MUTEX _mutex{};
};

} // namespace Robotiq::detail
