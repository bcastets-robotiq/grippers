// Copyright (c) 2026 Robotiq, Inc.
//
// Licensed under the BSD-3-Clause license; see LICENSE for details.

// Doc-only source: every //! [tag] region below backs a \snippet reference
// from a header's \code{.cpp} example (see EXAMPLE_PATH in ../Doxyfile).
// Nothing here is called from main() below — main() only exists so this
// compiles to a program; being compiled by GRIPPERS_BUILD_EXAMPLES, and thus
// checked by every normal build, is the whole point. Concepts already
// demonstrated by quick_start.cpp/move_gripper.cpp are tagged there instead;
// this file is only for the ones with no other home.

#include <Robotiq/gripper.hpp>
#include <Robotiq/gripper/fake/gripper_factory.hpp>
#include <Robotiq/gripper/named_bit_array.hpp>
#include <Robotiq/gripper/register_map.hpp>
#include <Robotiq/gripper/serial.hpp>
#include <Robotiq/gripper/throttle.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

using namespace std::chrono_literals;

namespace {

//! [configure-connection]
void configureConnection()
{
   Robotiq::ConnectionConfig config;
   config.modbusSlaveAddress = 0x09;
   config.serial.port = "/dev/ttyUSB0";
   config.serial.baudrate = 115200;
   config.connectionFrequency = 100.0; // Hz
}
//! [configure-connection]

void commandActionBits()
{
   //! [command-action-bits]
   // Command object use to interact with gripper holding registers
   Robotiq::GripperCommand command = Robotiq::GripperCommand::defaults();

   // Command building blocks
   // ACTION - rACT
   command.action.set(Robotiq::ActionRequestBit::Activate, true);
   // ACTION - rGTO
   command.action.set(Robotiq::ActionRequestBit::GoTo, true);
   // ACTION - rATR
   command.action.set(Robotiq::ActionRequestBit::AutoRelease, false);
   // ACTION - rARD
   command.action.set(Robotiq::ActionRequestBit::AutoReleaseOpenDirection, true);
   // POSITION REQUEST - rPR
   command.positionRequest = 100;
   // SPEED - rSP
   command.speed = 255;
   // FORCE - rFR
   command.force = 255;
   //! [command-action-bits]
}

void statusGripperStatusFields(Robotiq::Gripper& gripper)
{
   //! [status-gripper-status-fields]
   // Status object retrieved from gripper input registers
   Robotiq::GripperStatus status = gripper.getStatus();

   // Status building blocks
   // GRIPPER STATUS - gOBJ
   Robotiq::ObjectDetection gOBJ = status.gripperStatus.objectDetection();
   // GRIPPER STATUS - gSTA
   Robotiq::ActivationState gSTA = status.gripperStatus.activationState();
   // GRIPPER STATUS - gGTO
   bool gGTO = status.gripperStatus.goToEnabled();
   // GRIPPER STATUS - gACT
   bool gACT = status.gripperStatus.activated();
   // FAULT STATUS - kFLT
   Robotiq::ControllerFault kFLT = status.faultStatus.controllerFault();
   // FAULT STATUS - gFLT
   Robotiq::GripperFault gFLT = status.faultStatus.gripperFault();
   // POS REQUEST ECHO - gPR
   uint8_t gPR = status.positionRequestEcho;
   // POSITION - gPO
   uint8_t gPO = status.position;
   // CURRENT - gCU
   uint8_t gCU = status.current;
   //! [status-gripper-status-fields]
}

//! [action-request-bits]
void actionRequestBits(Robotiq::Gripper& gripper)
{
   Robotiq::GripperCommand command = Robotiq::GripperCommand::defaults();
   command.action.set(Robotiq::ActionRequestBit::GoTo);                // start moving (same as set(GoTo, true))
   command.action.set(Robotiq::ActionRequestBit::AutoRelease, false);  // ...but not an emergency release
   bool goTo = command.action.get(Robotiq::ActionRequestBit::GoTo);    // true
   gripper.setCommand(command);
   (void)goTo;
}
//! [action-request-bits]

//! [gripper-status-flags]
void gripperStatusFlags(Robotiq::Gripper& gripper)
{
   Robotiq::GripperStatus status = gripper.getStatus();
   bool activated = status.gripperStatus.activated(); // gACT flag
   bool goTo = status.gripperStatus.goToEnabled();     // gGTO flag
   if(status.gripperStatus.activationState() == Robotiq::ActivationState::Complete
      && status.gripperStatus.objectDetection() != Robotiq::ObjectDetection::Moving)
   {
      // motion has settled — status.position holds the final rPR (0..255)
   }
   (void)activated;
   (void)goTo;
}
//! [gripper-status-flags]

//! [motion-settled-example]
void motionSettledExample(Robotiq::Gripper& gripper)
{
   Robotiq::GripperStatus status = gripper.getStatus();
   if(status.gripperStatus.objectDetection() != Robotiq::ObjectDetection::Moving)
   {
      // motion has settled — status.position holds the final rPR (0..255)
   }
}
//! [motion-settled-example]

//! [fault-severity-check]
void faultSeverityCheck(Robotiq::Gripper& gripper)
{
   Robotiq::FaultStatus fault = gripper.getStatus().faultStatus;
   if(Robotiq::severity(fault.gripperFault()) == Robotiq::FaultSeverity::Major)
   {
      (void)Robotiq::recoverFromFault(gripper);
   }
}
//! [fault-severity-check]

//! [uart-logger]
class UartLogger : public Robotiq::Logger
{
public:
   void log(Level level, std::string_view message) override
   {
      if(level >= Level::Warn)
      {
         uartWrite("!! ");
      }
      uartWrite(message);
   }

private:
   static void uartWrite(std::string_view) {} // stands in for a real UART driver
};

void useCustomLogger()
{
   auto logger = std::make_shared<UartLogger>();
   Robotiq::ConnectionConfig config;
   config.serial.port = "/dev/ttyUSB0";
   Robotiq::Gripper gripper(config, logger);
}
//! [uart-logger]

//! [named-bit-array]
void namedBitArrayUsage()
{
   enum class Flag : uint8_t
   {
      A = 0,
      B = 1
   };
   Robotiq::NamedBitArray<Flag> bits;
   bits.set(Flag::A);
   bits.set(Flag::B, false);
   bool a = bits.get(Flag::A); // true
   (void)a;
}
//! [named-bit-array]

//! [make-fake-gripper]
void makeFakeGripperUsage()
{
   auto gripper = Robotiq::makeFakeGripper(); // no hardware needed
   (void)Robotiq::activate(*gripper);
}
//! [make-fake-gripper]

//! [decode-raw-status-byte]
void decodeRawStatusByte(uint8_t rawStatusByte)
{
   uint8_t gSTA = (rawStatusByte & Robotiq::register_map::kActivationStateMask)
                  >> Robotiq::register_map::kActivationStateShift;
   bool activated = (rawStatusByte & Robotiq::register_map::kActivationStatusMask) != 0;
   (void)gSTA;
   (void)activated;
}
//! [decode-raw-status-byte]

//! [loopback-serial]
class RingBuffer
{
public:
   std::vector<uint8_t> take(size_t) { return {}; }
   void feed(const std::vector<uint8_t>&) {}
};

class LoopbackSerial : public Robotiq::Serial
{
public:
   void open() override { _open = true; }
   bool isOpen() const override { return _open; }
   void close() override { _open = false; }
   std::vector<uint8_t> read(size_t size, std::chrono::milliseconds) override
   {
      return _rx.take(size); // however the test feeds bytes in
   }
   void write(const std::vector<uint8_t>& data) override { _rx.feed(data); }
   std::chrono::milliseconds getTimeout() const override { return _timeout; }

private:
   bool _open = false;
   std::chrono::milliseconds _timeout{500};
   RingBuffer _rx;
};
//! [loopback-serial]

//! [wait-with-platform]
void waitWithPlatform(Robotiq::Gripper& gripper, uint8_t target)
{
   bool settled = Robotiq::waitFor(
      [&] { return gripper.getStatus().positionRequestEcho == target; },
      gripper.platform(),
      std::chrono::seconds(1));
   (void)settled;
}
//! [wait-with-platform]

//! [throttle-usage]
void throttleUsage(Robotiq::Logger& logger, bool running)
{
   Robotiq::Throttle logThrottle(std::chrono::seconds(1));
   while(running)
   {
      logThrottle.executeIfAllowed([&] { logger.log(Robotiq::Logger::Level::Debug, "still running"); });
      running = false; // never actually loops; this function only needs to compile
   }
}
//! [throttle-usage]

} // namespace

int main()
{
   return 0;
}
