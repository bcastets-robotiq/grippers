// Copyright (c) 2026 Robotiq, Inc.
//
// Licensed under the BSD-3-Clause license; see LICENSE for details.

#pragma once

#include <cstdint>
#include <type_traits>

#include <Robotiq/gripper/register_map.hpp>

namespace Robotiq {

//! \ingroup fault_status
//! The gripper's own fault code (gFLT, low nibble of the FAULT STATUS byte).
enum class GripperFault : uint8_t
{
   None = 0x00, //!< no fault
   ActionDelayed = 0x05, //!< reactivation must complete before the action
   ActivationRequired = 0x07, //!< rACT must be set before an action
   OverTemperature = 0x08, //!< over max operating temperature; let it cool
   NoCommunication = 0x09, //!< no communication for at least 1 s
   UnderVoltage = 0x0A, //!< under minimum operating voltage
   AutomaticReleaseInProgress = 0x0B, //!< rATR emergency release running
   InternalFault = 0x0C, //!< contact support
   ActivationFault = 0x0D, //!< check for interference during activation
   Overcurrent = 0x0E, //!< motor overcurrent protection tripped
   AutomaticReleaseComplete = 0x0F, //!< rATR emergency release finished
};

//! \ingroup fault_status
//! The optional Robotiq controller's fault code (kFLT, high nibble of the FAULT STATUS byte).
enum class ControllerFault : uint8_t
{
   None = 0x00, //!< no fault
   Supply24VNotDetected = 0x04, //!< reconfiguration over USB still possible
   NoDeviceDetected = 0x05, //!< no gripper detected on the bus
   CommunicationNotReady = 0x09, //!< main communication protocol booting
   EmergencyStop = 0x0C, //!< emergency stop engaged
   Overcurrent = 0x0E, //!< controller overcurrent protection tripped
};

//! \ingroup fault_status
//! \brief How serious a fault is, per the gripper manual.
//!
//! The manual labels the Warning tier "priority faults"; a Major fault
//! needs a reset (a rising edge on rACT) to clear — see
//! recoverFromFault().
enum class FaultSeverity : uint8_t
{
   None, //!< no fault
   Warning, //!< informational; clears on its own
   Minor, //!< degraded operation; clears on its own
   Major, //!< needs a reset (rACT rising edge) to clear
};

//! \ingroup fault_status
//! \brief Classify a gripper fault code by severity.
//! \param fault The gripper's own fault code, from FaultStatus::gripperFault().
//! \return The fault's severity. Unrecognized codes report Major so an
//!         unknown fault is never mistaken for harmless.
[[nodiscard]] constexpr FaultSeverity severity(GripperFault fault)
{
   switch(fault)
   {
   case GripperFault::None:
      return FaultSeverity::None;
   case GripperFault::ActionDelayed:
   case GripperFault::ActivationRequired:
      return FaultSeverity::Warning;
   case GripperFault::OverTemperature:
   case GripperFault::NoCommunication:
      return FaultSeverity::Minor;
   case GripperFault::UnderVoltage:
   case GripperFault::AutomaticReleaseInProgress:
   case GripperFault::InternalFault:
   case GripperFault::ActivationFault:
   case GripperFault::Overcurrent:
   case GripperFault::AutomaticReleaseComplete:
      return FaultSeverity::Major;
   }
   return FaultSeverity::Major;
}

//! \ingroup fault_status
//! \brief Classify a controller fault code by severity.
//! \param fault The controller's fault code, from FaultStatus::controllerFault().
//! \return The fault's severity. Unrecognized codes report Major so an
//!         unknown fault is never mistaken for harmless.
[[nodiscard]] constexpr FaultSeverity severity(ControllerFault fault)
{
   switch(fault)
   {
   case ControllerFault::None:
      return FaultSeverity::None;
   case ControllerFault::Supply24VNotDetected:
   case ControllerFault::NoDeviceDetected:
      return FaultSeverity::Warning;
   case ControllerFault::CommunicationNotReady:
      return FaultSeverity::Minor;
   case ControllerFault::EmergencyStop:
   case ControllerFault::Overcurrent:
      return FaultSeverity::Major;
   }
   return FaultSeverity::Major;
}

//! \ingroup fault_status
//! \brief The FAULT STATUS byte (byte 2 of the status block).
//!
//! Splits into the gripper's own fault (gFLT, low nibble) and the
//! optional Robotiq controller's fault (kFLT, high nibble).
//!
//! \par Example
//! \code{.cpp}
//! Robotiq::FaultStatus fault = gripper.getStatus().faultStatus;
//! if(Robotiq::severity(fault.gripperFault()) == Robotiq::FaultSeverity::Major)
//! {
//!    Robotiq::recoverFromFault(gripper);
//! }
//! \endcode
class FaultStatus
{
public:
   //! \brief Synthesize a byte the gripper would have sent.
   //!
   //! For simulators and for exercising fault handling without hardware.
   //! \param bits The raw FAULT STATUS byte.
   //! \return A FaultStatus wrapping \p bits.
   [[nodiscard]] static constexpr FaultStatus fromRaw(uint8_t bits)
   {
      FaultStatus status;
      status._bits = bits;
      return status;
   }

   //! \return gFLT — the gripper's own fault code.
   [[nodiscard]] GripperFault gripperFault() const
   {
      return static_cast<GripperFault>(_bits & register_map::kGripperFaultMask);
   }

   //! \return kFLT — the optional Robotiq controller's fault code.
   [[nodiscard]] ControllerFault controllerFault() const
   {
      return static_cast<ControllerFault>((_bits & register_map::kControllerFaultMask)
                                          >> register_map::kControllerFaultShift);
   }

   //! \return The raw FAULT STATUS byte, unpacked.
   [[nodiscard]] uint8_t raw() const { return _bits; }

   //! \return true if both fault bytes are identical.
   [[nodiscard]] bool operator==(FaultStatus other) const { return _bits == other._bits; }
   //! \return true if the fault bytes differ.
   [[nodiscard]] bool operator!=(FaultStatus other) const { return _bits != other._bits; }

private:
   uint8_t _bits = 0;
};

static_assert(std::is_standard_layout_v<FaultStatus> && std::is_trivially_copyable_v<FaultStatus>
                 && sizeof(FaultStatus) == 1,
              "FaultStatus must be a single byte");
} // namespace Robotiq
