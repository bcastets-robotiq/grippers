# How it works

At the wire level, Robotiq grippers are controlled by writing commands to, and reading status from, their memory over Modbus RTU. With this SDK, though, you never issue Modbus RTU requests yourself: you call `setCommand()` and `getStatus()`, and the `Gripper` object handles the Modbus RTU exchange with the hardware in the background.

Check the gripper user manual on the Robotiq support website if you want to learn more about the gripper's Modbus RTU communication.

## Command

The registers used to command the gripper are composed of 3 16-bit registers.
Each register is split into 2 bytes, for a total of 6 bytes:

|Register| Byte | Name | Content |
|---|---|---|---|
|1000| 0 | ACTION REQUEST | Packed Byte |
|1000| 1 | Reserved | - |
|1001| 2 | Reserved | - |
|1001| 3 | POSITION REQUEST (rPR) | 0-255 value |
|1002| 4 | SPEED (rSP) | 0-255 value |
|1002| 5 | FORCE (rFR) | 0-255 value |

The gripper action request byte is a packed bit field.

| Bits | 7-6 | 5 | 4 | 3 | 2-1 | 0 |
|---|---|---|---|---|---|---|
| Field | reserved | rARD | rATR | rGTO | reserved | rACT |

This SDK translates this into a command with equivalent fields.

Packed bytes are built using the `set` function and a dedicated enum.

<!-- snippet: snippets.cpp command-action-bits -->
```cpp
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
```

## Status

The registers used to retrieve the status of the gripper are composed of 3 registers of 16 bits. Each register is split into 2 bytes (8 bits), for a total of 6 bytes.

|Register| Byte | Name | Content |
|---|---|---|---|
|2000| 0 | GRIPPER STATUS | Packed Byte |
|2000| 1 | Reserved | - |
|2001| 2 | FAULT STATUS | Packed Byte |
|2001| 3 | POS REQUEST ECHO (gPR) | 0-255 value |
|2002| 4 | POSITION (gPO) | 0-255 value |
|2002| 5 | CURRENT (gCU) | 0-255 value |

The positionRequestEcho, position and current bytes are unsigned integers coded on 8 bits, with a value in the range 0-255.

The gripper status and fault status bytes are packed bit fields.

*GRIPPER STATUS BYTE (0)*

| Bits | 7-6 | 5-4 | 3 | 2-1 | 0 |
|---|---|---|---|---|---|
| Field | gOBJ | gSTA | gGTO | reserved | gACT |

*FAULT STATUS BYTE (2)*

| Bits | 7-4 | 3-0 |
|---|---|---|
| Field | kFLT | gFLT |

This SDK translates this into a status object with equivalent fields, decoded using a dedicated enum.

<!-- snippet: snippets.cpp status-gripper-status-fields -->
```cpp
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
```

## Gripper-related functions

`Gripper`'s own methods — `setCommand()`, `getStatus()`, `getCommand()`,
`connectionState()` — are used to set the command or read the status of the gripper.

> **Warning:** calling `setCommand()` does not necessarily mean the command
> will effectively be sent to the gripper.

Those functions only read or write a local copy of the gripper's Modbus
registers, owned by the gripper object. They do not send any Modbus RTU
command to the gripper. The Modbus RTU communication is managed in the
background by the gripper object, which runs a continuous communication
thread with the gripper.

As a consequence, if you write back-to-back `setCommand()` instructions, only the latest one will be taken into account.

As an example, the code below sets an autorelease command and, right after that, a move command.

<!-- snippet: snippets.cpp autorelease-then-move -->
```cpp
// Build and set an autorelease command
Robotiq::GripperCommand command = Robotiq::GripperCommand::defaults();
command.action.set(Robotiq::ActionRequestBit::AutoRelease);
gripper.setCommand(command);

// Build and set a command to move the gripper to the position 100
command = Robotiq::GripperCommand::defaults();
command.action.set(Robotiq::ActionRequestBit::GoTo);
command.positionRequest = 100;
gripper.setCommand(command);
```

If the autorelease command is effectively sent to the gripper, the gripper will
execute the autorelease, which has the effect of opening or closing the gripper
and deactivating it. As a consequence, it is not possible to move the
gripper after the autorelease.

Looking at this code, you may think that the second command, asking for the gripper to move to position 100, will probably not be executed, but in fact it will be. The first `setCommand()` writes the autorelease command to the local copy of the Modbus register, and it is immediately followed by another `setCommand()` which rewrites the local copy before it is effectively sent to the gripper. The consequence is that the autorelease command is not sent to the gripper, and the move command is executed instead.

To have the autorelease command effectively sent to the gripper, it is necessary to wait for the gripper to acknowledge reception of the command before the next `setCommand()`.

<!-- snippet: snippets.cpp autorelease-then-move-with-wait -->
```cpp
// Build and set an autorelease command
Robotiq::GripperCommand command = Robotiq::GripperCommand::defaults();
command.action.set(Robotiq::ActionRequestBit::AutoRelease);
gripper.setCommand(command);

// Wait
Robotiq::waitFor(
   [&] { return (gripper.getStatus().faultStatus.gripperFault() == Robotiq::GripperFault::AutomaticReleaseInProgress); }, 10s);

// Build and set a command to move the gripper to the position 100
command = Robotiq::GripperCommand::defaults();
command.action.set(Robotiq::ActionRequestBit::GoTo);
command.positionRequest = 100;
gripper.setCommand(command);
```

The following section presents the wait function used in the code above.

### Waiting for a condition

`waitFor()` (and `waitUntil()`, which takes a deadline instead of a
timeout) polls a predicate until it becomes true or the timeout
elapses:

<!-- snippet: snippets.cpp wait-for-motion-settled -->
```cpp
bool settled = Robotiq::waitFor(
   [&] { return gripper.getStatus().gripperStatus.objectDetection() != Robotiq::ObjectDetection::Moving; },
   10s);
```

The predicate is a C++ lambda — an anonymous inline function. `[&]` captures
`gripper` (and any other locals it uses) by reference, so the body reads
live state each time it's polled, and it must return a `bool`: `true` once
the condition is met.

It returns `true` if the condition held before the timeout, `false`
otherwise — the condition is checked at least once, so an already-true
condition never reports a timeout.

### Activating the gripper

Before sending motion commands, the gripper must be activated once.
`activate()` is a blocking function that runs the activation handshake
(or waits out one already in progress):

<!-- snippet: snippets.cpp activate-only -->
```cpp
Robotiq::ActivationResult result = Robotiq::activate(gripper);
```

| `ActivationResult` | Meaning |
|---|---|
| `Activated` | the handshake ran (or one already in progress finished); the gripper is ready |
| `AlreadyActive` | already activated and fault-free; `activate()` did nothing |
| `FaultLatched` | a major fault is latched; `activate()` refuses to reset it — call `recoverFromFault()` instead |
| `Timeout` | the link stayed down, or the handshake never completed in time |

Calling `activate()` on an already-activated gripper is safe and a
no-op, so it's fine to call it at the start of every run rather than
tracking activation state yourself.

### Recovering from a fault

If `activate()` returns `FaultLatched`, or a `Major` fault shows up
later during operation (see [Error handling](#error-handling) below),
call `Robotiq::recoverFromFault()`:

<!-- snippet: snippets.cpp recover-from-fault-only -->
```cpp
Robotiq::ActivationResult result = Robotiq::recoverFromFault(gripper);
```

`recoverFromFault()` clears the activation bit (rACT) — which resets
the gripper and clears its fault status — then sets it back to rerun
the activation handshake, blocking until it completes.

> **Warning:** this releases any grip and sweeps the fingers through
> their full range.

## Error handling

Constructing a `Gripper` can throw:
- `SerialIOException` — the serial port could not be opened or configured.
- `DriverException` — no gripper answered the initial read, or the
  connection configuration (e.g. `connectionFrequency`) is invalid.

### Checking fault severity

Not every fault needs recovery. `Robotiq::severity()` classifies the
raw fault code from `status.faultStatus` into a `FaultSeverity`:

<!-- snippet: snippets.cpp fault-severity-check -->
```cpp
Robotiq::FaultStatus fault = gripper.getStatus().faultStatus;
if(Robotiq::severity(fault.gripperFault()) == Robotiq::FaultSeverity::Major)
{
   Robotiq::recoverFromFault(gripper);
}
```

| `FaultSeverity` | Meaning |
|---|---|
| `None` | no fault |
| `Warning` | informational; clears on its own |
| `Minor` | degraded operation; clears on its own |
| `Major` | needs a reset (rACT rising edge) to clear — call [`recoverFromFault()`](#recovering-from-a-fault) |

## Control method

The communication flow to control the gripper is typically the following:
- Build a command
- Send the command to the gripper
- Wait for the gripper to acknowledge the command
- Wait for the gripper to complete the action
- Check final status

Build a command:

<!-- snippet: quick_start.cpp qs-create-command -->
```cpp
Robotiq::GripperCommand command = Robotiq::GripperCommand::defaults();
command.action.set(Robotiq::ActionRequestBit::GoTo);
command.positionRequest = 100;
command.speed = 255;
command.force = 255;
```

Send the command:

<!-- snippet: quick_start.cpp qs-send-command -->
```cpp
gripper.setCommand(command);
```

Wait for the gripper to acknowledge the command, then wait for it to complete:

<!-- snippet: quick_start.cpp qs-wait -->
```cpp
// 6- Wait for the gripper to echo
Robotiq::waitFor([&]{return gripper.getStatus().positionRequestEcho == command.positionRequest;},1s);

// 7- Wait for the action to complete
Robotiq::waitFor([&]{return (gripper.getStatus().gripperStatus.objectDetection() != Robotiq::ObjectDetection::Moving);},10s);
```

Check final status:

<!-- snippet: quick_start.cpp qs-status -->
```cpp
// 8- retrieve status
uint8_t currentPosition = gripper.getStatus().position;

// Print retrieved status
std::cout << "Current position : " << static_cast<int>(currentPosition) << std::endl;
```

## Without a gripper

`makeFakeGripper()` returns a `Gripper` driving a fake device instead of
a serial port, for bring-up, demos, and CI on machines with no hardware
attached:

<!-- snippet: snippets.cpp make-fake-gripper -->
```cpp
auto gripper = Robotiq::makeFakeGripper(); // no hardware needed
Robotiq::activate(*gripper);
```

Everything above the wire is the real thing — the typed blocks, the
exchange cycle, the process image, `activate()` / `recoverFromFault()`.
The device below it is deliberately minimal: activation completes
instantly, and the fingers are wherever they were last commanded to be.
There is no motion profile, no travel time, no object detection, and no
fault injection.

That makes it a good fit for exercising your own integration logic
against the SDK's real API surface — including in CI, with no hardware
attached — but it can't stand in for a physical gripper when what
you're validating is motion timing, object detection, or fault
behavior.

`makeFakeGripper()` is built under the `GRIPPERS_BUILD_FAKE` CMake
option; see [CMake options](1-Environment%20setup.md#cmake-options).

## How the C++ driver handles communication with the gripper

The C++ driver has been developed with the objective of maximizing communication frequency.

The `Gripper` object owns a background thread that continuously exchanges
FC 0x17 Modbus (read&write) transactions with the gripper — up to ~200 Hz at 115200 baud. That thread is the only thing that directly communicates with the gripper.

## Logging

`Gripper`'s constructor takes an optional `logger` parameter (a
`std::shared_ptr<Robotiq::Logger>`). Passing nothing, or explicitly
`nullptr`, doesn't disable logging — it falls back to the SDK's own
default logger: `StderrLogger` on a hosted build, a do-nothing
`NullLogger` on a freestanding target with no console.

<!-- snippet: snippets.cpp basic-logger-injection -->
```cpp
auto logger = std::make_shared<Robotiq::StderrLogger>();
Robotiq::Gripper gripper(config, logger);
```

`Logger` is a one-method interface — `log(Level, message)` — with four severities:

| `Robotiq::Logger::Level` | Meaning |
|---|---|
| `Debug` | high-frequency, diagnostic detail |
| `Info` | normal operational events |
| `Warn` | recoverable, but worth a human's attention |
| `Error` | an operation failed |

The background exchange thread (above) uses this same logger to report
its own health, independently of anything your own code does:
- `Warn` when several consecutive exchanges fail — the same moment
  `connectionState()` switches to `ConnectionState::Faulted`, meaning
  the process image `getStatus()` returns is now stale.
- `Info` when the link recovers and `connectionState()` returns to
  `Operational`.

Passing your own `Logger` (e.g. one that forwards to `rclcpp` in a
ROS 2 node, or writes to a UART on an embedded target) lets you route
these lines — and your own application's — through one sink instead of
two independently-timed sources that could interleave confusingly, the
same pattern
[`move_gripper.cpp` uses](4-Robust%20example%20walkthrough.md#sharing-one-logger)
for its own narration.