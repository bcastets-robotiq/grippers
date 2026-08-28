// Copyright (c) 2026 Robotiq, Inc.
//
// Licensed under the BSD-3-Clause license; see LICENSE for details.

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>

#include <Robotiq/detail/config.hpp>
#include <Robotiq/gripper/platform.hpp>
#include <Robotiq/gripper/connection_config.hpp>
#include <Robotiq/gripper/connection_state.hpp>
#include <Robotiq/gripper/logger.hpp>
#include <Robotiq/gripper/command.hpp>
#include <Robotiq/gripper/status.hpp>
#include <Robotiq/gripper/wait.hpp>

namespace Robotiq {
class Serial;

//! \ingroup core_api
//! \brief The Robotiq gripper API.
//!
//! Construction opens the link, reads the gripper — failing like a dead
//! serial link when it does not answer — and starts the exchange cycle;
//! destruction stops it. Control is instant: setCommand() and getStatus()
//! access an internal process image and never touch the bus. The control
//! image seeds from the gripper's own status echoes, so connecting never
//! disturbs a gripper that is already running.
//!
//! **Concurrency model:** accessors are thread-safe; the intended use is
//! one control thread writing commands while a background exchange
//! thread reads/writes the wire. Reads are whole snapshots and writes are
//! whole commands — no per-field accessors, deliberately: every
//! transmitted frame is a command the application composed, and two
//! fields never come from different exchange cycles.
//!
//! Blocking procedures — activate(), recoverFromFault() — are free
//! functions, not members, even though they're specifically meant to be
//! used with this class: every *member* of Gripper is guaranteed to
//! return instantly, and keeping the blocking procedures outside the
//! class is what signals the difference. They're composed entirely out
//! of the same public accessors (getStatus(), setCommand()) your own
//! code has, polling in a loop — which also means they have no way to
//! hold the internal lock those accessors use for longer than an
//! instant, the way an equivalent member function easily could.
//!
//! \par Two constructors, for two different situations
//! These are two independent overloads, not one constructor with more
//! optional parameters — the compiler picks between them at compile time,
//! from the number and types of arguments you pass. The
//! `ConnectionConfig`-based overload is compiled only when
//! `GRIPPERS_BUILD_DEFAULT_SERIAL` is `1`; the `Serial`/`Platform`-based
//! overload is always available and is the path to use when that macro is
//! `0`:
//! - The `ConnectionConfig`-based constructor below is the common case:
//!   a desktop app talking to a gripper over a real serial port. Its
//!   `logger` is that constructor's *2nd* parameter.
//! - The `Serial`/`Platform`-based constructor further below is for
//!   everything else that isn't that: a custom transport, a unit test
//!   double, or a freestanding/RTOS target with no libserialport. Its
//!   `logger` is that constructor's *5th* parameter — a different
//!   parameter of a different function, even though both happen to be
//!   named `logger` and both default to `nullptr` (meaning "use the
//!   SDK's own default logger", not "no logging").
//!
//! See the SDK documentation's "How it works" page for the full connect →
//! activate → command → status walkthrough, with the field-by-field
//! reference for GripperCommand and GripperStatus, and "Embedded /
//! bare-metal builds" for when the second constructor applies.
class Gripper
{
public:
#if GRIPPERS_BUILD_DEFAULT_SERIAL
   //! \brief Open a gripper over the built-in serial transport.
   //!
   //! Available only when `GRIPPERS_BUILD_DEFAULT_SERIAL` is `1`.
   //! This is normally enabled for hosted desktop builds and disabled for
   //! freestanding or RTOS builds.
   //! The common-case constructor: a real gripper over a real serial port.
   //! \param config Serial link and Modbus addressing; see ConnectionConfig.
   //! \param logger Log sink; pass null to use the default stderr logger.
   //! \throw SerialIOException when the port cannot be opened/configured.
   //! \throw DriverException when no gripper answers the initial read, or
   //!        when config.connectionFrequency is invalid.
   //!
   //! For custom transports, test doubles, and freestanding/RTOS targets,
   //! use the other constructor below instead.
   explicit Gripper(const ConnectionConfig& config, std::shared_ptr<Logger> logger = nullptr);
#endif

   //! \brief Open a gripper over a caller-supplied transport and platform.
   //!
   //! Available regardless of `GRIPPERS_BUILD_DEFAULT_SERIAL`; use this
   //! overload when the built-in libserialport transport is disabled.
   //! For custom serial implementations, unit tests, and RTOS targets. The
   //! exchange runs on the given platform — makeDefaultPlatform() on a
   //! hosted runtime, or your RTOS port (see Platform and ports/).
   //! \param serial The transport to exchange over; must not be null.
   //! \param slaveAddress The gripper's Modbus slave address.
   //! \param exchangePeriod Period of the background exchange cycle.
   //! \param platform Runtime services (thread, lock, sleep) for the
   //!        exchange to run on; must not be null.
   //! \param logger Log sink; pass null to use the build's default logger.
   //! \throw SerialIOException when the port cannot be opened/configured.
   //! \throw DriverException when no gripper answers the initial read, when
   //!        exchangePeriod is invalid, or when platform is null.
   //!
   //! For the common case — a real gripper over a real serial port — use
   //! the other constructor above instead.
   Gripper(std::unique_ptr<Serial> serial,
           uint8_t slaveAddress,
           std::chrono::microseconds exchangePeriod,
           std::shared_ptr<Platform> platform,
           std::shared_ptr<Logger> logger = nullptr);

   //! Stops the exchange cycle and closes the link.
   ~Gripper();

   Gripper(const Gripper&) = delete;
   Gripper& operator=(const Gripper&) = delete;

   //! \brief Send a new command block on the next exchange cycle.
   //! \param command The whole command block to transmit; see GripperCommand.
   void setCommand(const GripperCommand& command);

   //! \return The last command block passed to setCommand() — or the
   //!         gripper's own echoed state, before the first call.
   //! \note [[nodiscard]]: a pure snapshot read with no side effects; calling
   //!       it only to discard the result is always a mistake.
   [[nodiscard]] GripperCommand getCommand() const;

   //! \return A snapshot of the gripper's last received status block.
   //! \note [[nodiscard]]: a pure snapshot read with no side effects; calling
   //!       it only to discard the result is always a mistake.
   [[nodiscard]] GripperStatus getStatus() const;

   // TODO: add an exchange-cycle sync primitive so a caller's control loop
   // can run in step with the background exchange without polling

   // TODO: add debug helpers rendering the command and status blocks in
   // a human-readable form (named fields, decoded bits and fault codes)

   //! \return The current state of the background exchange; see ConnectionState.
   //! \note [[nodiscard]]: a pure snapshot read with no side effects; calling
   //!       it only to discard the result is always a mistake.
   [[nodiscard]] ConnectionState connectionState() const;

   //! \brief Return the runtime Platform used by this gripper.
   //!
   //! The platform supplies the runtime services needed by the background
   //! exchange thread: spawning a thread, creating its lock, and yielding
   //! while waiting. It is selected when the Gripper is constructed —
   //! `makeDefaultPlatform()` for a hosted application, or a caller-supplied
   //! RTOS implementation for an embedded target.
   //!
   //! Use this accessor when code outside Gripper needs to wait or sleep in
   //! the same runtime environment. For example, pass it to the Platform
   //! overload of `waitFor()` when polling a status condition. The returned
   //! reference is owned by Gripper and remains valid until that Gripper is
   //! destroyed; callers must not delete or replace it. This function does
   //! not create a new platform and does not provide direct serial access.
   //!
   //! \par Example
   //! \code{.cpp}
   //! bool settled = Robotiq::waitFor(
   //!    [&] { return gripper.getStatus().positionRequestEcho == target; },
   //!    gripper.platform(),
   //!    std::chrono::seconds(1));
   //! \endcode
   //! \note [[nodiscard]]: a pure accessor with no side effects; calling it
   //!       only to discard the result is always a mistake.
   [[nodiscard]] Platform& platform() const noexcept;

private:
   struct Impl; // hides the link, the exchange thread, and the image
   std::unique_ptr<Impl> _impl;
};

//! \ingroup activation
//! Result of the blocking activation procedures activate() and recoverFromFault().
enum class ActivationResult
{
   Activated, //!< the gripper reports activation complete — the handshake
              //!< ran, or one already under way finished
   AlreadyActive, //!< already activated and fault-free; nothing was sent
   FaultLatched, //!< a major fault is latched; activate() refuses the reset
   Timeout, //!< the link stayed down, completion never arrived in time, or
            //!< too little of the timeout remained to run the handshake
};

//! \ingroup activation
//! \brief Ensure the gripper is activated, blocking until it reports completion.
//!
//! A healthy, already-activated gripper is left undisturbed, and an
//! activation already in progress is waited on rather than restarted.
//! rGTO is cleared when the handshake runs.
//! \param gripper The gripper to activate.
//! \param timeout How long to wait for the handshake to complete.
//! \return Activated or AlreadyActive on success; FaultLatched if a major
//!         fault blocks activation — call recoverFromFault() instead;
//!         Timeout if completion never arrived in time.
//! \note [[nodiscard]]: discarding the result silently misses FaultLatched
//!       and Timeout — the caller would have no way to tell success from
//!       a fault or a timed-out handshake.
//!
//! \par Example
//! \code{.cpp}
//! if(Robotiq::activate(gripper) == Robotiq::ActivationResult::FaultLatched)
//! {
//!    Robotiq::recoverFromFault(gripper);   // clears the fault; moves the fingers
//! }
//! \endcode
[[nodiscard]] ActivationResult activate(Gripper& gripper, std::chrono::milliseconds timeout = std::chrono::seconds(15));

//! \ingroup activation
//! \brief Run the manual's fault-recovery handshake, unconditionally.
//!
//! Clearing rACT resets the gripper — clearing its fault status — and
//! setting it back runs the calibration sweep.
//! \warning Releases any grip and moves the fingers through their full
//!          range. rGTO is cleared, as for activate().
//! \param gripper The gripper to recover.
//! \param timeout How long to wait for the handshake to complete.
//! \return Activated on success; Timeout if completion never arrived in time.
//! \note [[nodiscard]]: discarding the result silently misses Timeout — the
//!       caller would have no way to tell the handshake actually completed.
[[nodiscard]] ActivationResult recoverFromFault(Gripper& gripper,
                                                std::chrono::milliseconds timeout = std::chrono::seconds(15));
} // namespace Robotiq
