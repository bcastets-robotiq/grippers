// Copyright (c) 2026 Robotiq, Inc.
//
// Licensed under the BSD-3-Clause license; see LICENSE for details.

//! \brief A minimal std::thread-shaped thread + a monotonic sleep, so the
//! threaded runtime (Robotiq::Gripper) can run without a hosted C++ runtime.
//! Selected at compile time:
//!   1. hosted threaded runtime -> std::thread / std::this_thread;
//!   2. GRIPPERS_RTOS_THREADX    -> a ThreadX task wrapper;
//!   3. otherwise -> a hard error: the threaded Gripper needs threads; a
//!      single-threaded target should drive detail::GripperModbusClient itself.
//!
//! The ThreadX path allocates the task's stack and control block on the heap and
//! passes the callable (not `this`) as the entry argument, so a detail::Thread is
//! safely movable while the task is running. Stack size and priority are compile
//! time (GRIPPERS_THREADX_STACK_SIZE / _PRIORITY) — tune for your app; the entry
//! and the gripper exchange loop use exceptions + iostream, so keep the stack
//! generous. Construct after the ThreadX kernel is running.
//!
//! RTOS integration caveats (each cost real debugging time):
//!  - The injected Serial::read MUST yield the CPU while awaiting bytes — e.g.
//!    interrupt/DMA completion signalled through an RTOS semaphore. A busy-wait
//!    read (such as a polled HAL_UART_Receive) runs at this exchange thread's
//!    priority and will STARVE lower-priority tasks — including the app task that
//!    drives activate()/setCommand — hanging the application. This is not optional
//!    on a preemptive RTOS.
//!  - Choose GRIPPERS_THREADX_PRIORITY relative to your app task deliberately (the
//!    exchange thread and the app task must both make progress).
//!  - steady_clock needs a real monotonic backing (see sleepUntil below).

#pragma once

#include <chrono>

#include <Robotiq/detail/std_threading.hpp>

#if GRIPPERS_HAS_STD_THREADS

#include <thread>

namespace Robotiq::detail {
using Thread = std::thread;
inline void sleepUntil(std::chrono::steady_clock::time_point tp)
{
   std::this_thread::sleep_until(tp);
}
} // namespace Robotiq::detail

#elif defined(GRIPPERS_RTOS_THREADX)

#include <cstdint>
#include <functional>
#include <utility>

#include "tx_api.h"

#ifndef GRIPPERS_THREADX_STACK_SIZE
#define GRIPPERS_THREADX_STACK_SIZE 4096u
#endif
#ifndef GRIPPERS_THREADX_PRIORITY
#define GRIPPERS_THREADX_PRIORITY 15u // app-dependent: must fit your TX priority scheme
#endif

namespace Robotiq::detail {

// std::thread-shaped wrapper over a ThreadX task. Supports the subset Gripper
// uses: construct-with-callable (auto-start), move, joinable(), join().
class Thread
{
public:
   Thread() = default;

   template <typename Fn>
   explicit Thread(Fn&& fn)
      : _fn(new std::function<void()>(std::forward<Fn>(fn)))
      , _stack(new std::uint8_t[GRIPPERS_THREADX_STACK_SIZE])
      , _tcb(new TX_THREAD{})
   {
      // Entry argument is the callable pointer (stable across moves), never `this`.
      tx_thread_create(_tcb,
                       const_cast<CHAR*>("gripper"),
                       &Thread::entry,
                       reinterpret_cast<ULONG>(_fn),
                       _stack,
                       GRIPPERS_THREADX_STACK_SIZE,
                       GRIPPERS_THREADX_PRIORITY,
                       GRIPPERS_THREADX_PRIORITY,
                       TX_NO_TIME_SLICE,
                       TX_AUTO_START);
   }

   Thread(Thread&& other) noexcept { steal(other); }
   Thread& operator=(Thread&& other) noexcept
   {
      if(this != &other)
      {
         finalize();
         steal(other);
      }
      return *this;
   }

   Thread(const Thread&) = delete;
   Thread& operator=(const Thread&) = delete;

   ~Thread() { finalize(); }

   [[nodiscard]] bool joinable() const noexcept { return _tcb != nullptr; }

   // Block until the task's entry returns, then release its resources.
   void join()
   {
      if(_tcb == nullptr)
      {
         return;
      }
      UINT state = TX_READY;
      do
      {
         // nullptr, not TX_NULL: C++ won't convert (void*)0 to the typed out-params.
         tx_thread_info_get(_tcb, nullptr, &state, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
         if(state == TX_COMPLETED || state == TX_TERMINATED)
         {
            break;
         }
         tx_thread_sleep(1);
      } while(true);
      release();
   }

private:
   static void entry(ULONG arg) { (*reinterpret_cast<std::function<void()>*>(arg))(); }

   void steal(Thread& o) noexcept
   {
      _fn = o._fn;
      _stack = o._stack;
      _tcb = o._tcb;
      o._fn = nullptr;
      o._stack = nullptr;
      o._tcb = nullptr;
   }

   void release() noexcept
   {
      if(_tcb != nullptr)
      {
         tx_thread_delete(_tcb);
         delete _tcb;
         _tcb = nullptr;
      }
      delete[] _stack;
      _stack = nullptr;
      delete _fn;
      _fn = nullptr;
   }

   // Destroying a still-running task is a logic error (std::thread would
   // std::terminate); terminate it defensively, then release.
   void finalize() noexcept
   {
      if(_tcb != nullptr)
      {
         tx_thread_terminate(_tcb);
      }
      release();
   }

   std::function<void()>* _fn = nullptr;
   std::uint8_t* _stack = nullptr;
   TX_THREAD* _tcb = nullptr;
};

// Sleep until a monotonic time point. steady_clock must be backed on the target;
// the wait itself yields the CPU via the ThreadX scheduler. Granularity is one
// ThreadX tick.
//
// IMPORTANT: which libc call libstdc++ uses for steady_clock is toolchain/multilib
// dependent — it may be clock_gettime(CLOCK_MONOTONIC) OR gettimeofday(). Confirm
// in the disassembly and back that one (or both) from a monotonic tick source; a
// stubbed backing silently FREEZES steady_clock, which breaks every timeout and
// this loop's pacing (the failure looks like a hang inside activate(), not a clock
// bug). On arm-none-eabi 13.x / v8-m.main multilib it was gettimeofday().
inline void sleepUntil(std::chrono::steady_clock::time_point tp)
{
   const auto now = std::chrono::steady_clock::now();
   if(tp <= now)
   {
      return;
   }
   const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tp - now).count();
   const long long perSec = TX_TIMER_TICKS_PER_SECOND;
   unsigned long ticks = static_cast<unsigned long>((ns * perSec + 999999999LL) / 1000000000LL);
   if(ticks == 0)
   {
      ticks = 1;
   }
   tx_thread_sleep(ticks);
}

} // namespace Robotiq::detail

#else
#error                                                                                                                 \
   "Robotiq::Gripper needs threads: define GRIPPERS_RTOS_THREADX (or another RTOS adapter), or use detail::GripperModbusClient on a single-threaded target."
#endif
