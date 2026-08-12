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
//! from the number and types of arguments you pass:
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
   [[nodiscard]] GripperCommand getCommand() const;

   //! \return A snapshot of the gripper's last received status block.
   [[nodiscard]] GripperStatus getStatus() const;

   // TODO: add an exchange-cycle sync primitive so a caller's control loop
   // can run in step with the background exchange without polling

   // TODO: add debug helpers rendering the command and status blocks in
   // a human-readable form (named fields, decoded bits and fault codes)

   //! \return The current state of the background exchange; see ConnectionState.
   [[nodiscard]] ConnectionState connectionState() const;

   //! \brief The Platform this gripper runs on.
   //!
   //! For composing blocking helpers (as activate() does) that must sleep
   //! the way this gripper's target sleeps.
   [[nodiscard]] Platform& platform() const noexcept;

private:
   struct Impl; // hides the link, the exchange thread, and the image
   std::unique_ptr<Impl> _impl;
};

//! \ingroup core_api
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

//! \relatesalso Gripper
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
//!
//! \par Example
//! \code{.cpp}
//! if(Robotiq::activate(gripper) == Robotiq::ActivationResult::FaultLatched)
//! {
//!    Robotiq::recoverFromFault(gripper);   // clears the fault; moves the fingers
//! }
//! \endcode
[[nodiscard]] ActivationResult activate(Gripper& gripper, std::chrono::milliseconds timeout = std::chrono::seconds(15));

//! \relatesalso Gripper
//! \brief Run the manual's fault-recovery handshake, unconditionally.
//!
//! Clearing rACT resets the gripper — clearing its fault status — and
//! setting it back runs the calibration sweep.
//! \warning Releases any grip and moves the fingers through their full
//!          range. rGTO is cleared, as for activate().
//! \param gripper The gripper to recover.
//! \param timeout How long to wait for the handshake to complete.
//! \return Activated on success; Timeout if completion never arrived in time.
[[nodiscard]] ActivationResult recoverFromFault(Gripper& gripper,
                                                std::chrono::milliseconds timeout = std::chrono::seconds(15));
} // namespace Robotiq
