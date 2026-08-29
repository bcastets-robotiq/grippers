# Robust example walkthrough

[Quick start](2-Quick%20start.md) shows the bare minimum to move a
gripper and explicitly skips error handling. [`move_gripper.cpp`](../sdk_cpp/examples/move_gripper.cpp)
is the same connect → activate → command → wait → status flow made
robust, using the functions from [Introduction to gripper control](3-Introduction%20to%20gripper%20control.md).
This page doesn't re-walk that flow — it covers what's different about
the robust version and why.

## Argument handling

```cpp
if(argc < 2)
{
   std::cerr << "Usage: " << argv[0] << " <port> [baudrate]\n";
   return EXIT_FAILURE;
}
```

The port is required; the baudrate is optional and defaults to
`ConnectionConfig`'s own default (115200) if omitted. When a baudrate
is given, it's parsed and range-checked before use:

```cpp
constexpr unsigned long kMinBaudrate = 1;
constexpr unsigned long kMaxBaudrate = 1000000;
...
const unsigned long parsed = std::stoul(argv[2]);
if(parsed < kMinBaudrate || parsed > kMaxBaudrate)
{
   throw std::out_of_range("baudrate outside the supported range");
}
```

The lower bound isn't just documentation-by-example: `std::stoul("-1")`
doesn't throw, it silently wraps around to a huge unsigned value.
Without the `< kMinBaudrate` check, a typo'd negative baudrate would
sail past `std::stoul` and only fail (or misbehave) much later, further
from the actual mistake.

## Reporting a failed connection

Constructing `Gripper` can throw (`SerialIOException` or
`DriverException` — see [Error handling](3-Introduction%20to%20gripper%20control.md#error-handling)).
The example catches it once, right at construction, and turns it into a
checklist instead of a raw exception message:

```cpp
catch(const std::exception& ex)
{
   std::cerr << "Error: " << ex.what() << "\n\n"
             << "Could not open a gripper on '" << argv[1] << "'. Check that:\n"
             << "  - the gripper is connected and powered;\n"
             << "  - the port name is correct (Linux /dev/ttyUSB0, macOS /dev/tty.usbserial-*, Windows COM3);\n"
             << "  - you have permission to use it (Linux: join the 'dialout' group).\n";
   return EXIT_FAILURE;
}
```

That checklist covers the three things that actually cause this
exception in practice: nothing plugged in, the wrong port name for the
platform, and (Linux only) not being in the `dialout` group — see
[Serial port notes](1-Environment%20setup.md#serial-port-notes).

## Sharing one logger

```cpp
auto logger = std::make_shared<Robotiq::StderrLogger>();
gripper = std::make_unique<Gripper>(config, logger);
```

The SDK logs through an injectable `Logger`; the example passes the
same instance to the SDK *and* uses it for its own narration ("Activating...",
"Opening...", "Closing..."). That's not just for convenience — it's
what keeps the SDK's own log lines and the example's own progress
messages on one ordered stream, instead of two independently-timed
sources that could interleave confusingly.

## Handling a latched fault at activation

Quick start's activation note shows how to force a gripper into a
deactivated state. `move_gripper.cpp` instead handles the case where
activation can't proceed at all because a fault is already latched:

```cpp
ActivationResult activation = Robotiq::activate(*gripper);
if(activation == ActivationResult::FaultLatched)
{
   activation = Robotiq::recoverFromFault(*gripper);
}
if(activation != ActivationResult::Activated && activation != ActivationResult::AlreadyActive)
{
   return EXIT_FAILURE;
}
```

The SDK never calls `recoverFromFault()` for you implicitly, because it
releases any grip and sweeps the fingers through their full range —
motion the caller needs to expect. Falling through to `EXIT_FAILURE`
when the result is neither `Activated` nor `AlreadyActive` also
catches a `Timeout` from either call, which a check for `FaultLatched`
alone would miss.

## The `moveTo()` helper's three waits

Sending a `GoTo` command doesn't mean the move is done, or even that
the gripper received it. `moveTo()` waits in three stages, each
catching a different way that assumption could be wrong:

```cpp
if(!Robotiq::waitFor([&] { return gripper.getStatus().positionRequestEcho == position; }, 1s))
{
   return false; // the gripper never even acknowledged the request
}
if(!Robotiq::waitFor([&] { return gripper.getStatus().gripperStatus.objectDetection() == ObjectDetection::Moving; },
                     200ms))
{
   // no motion seen within 200 ms — only advisory, see below
}
if(!Robotiq::waitFor([&] { return motionSettled(gripper); }, 5s))
{
   return false; // motion started (or the check above just missed it) but never settled
}
```

1. **Wait for the position-request echo** (`positionRequestEcho`) —
   confirms the gripper actually received this command, as opposed to
   it being lost or still in flight. A timeout here fails the move.
2. **Wait (briefly) to see `objectDetection() == Moving`** — advisory
   only, and its own timeout is *not* treated as failure. `objectDetection()`
   can lag the echo by a few exchange cycles, and a short move can
   finish before it's ever observed mid-motion — so "never saw it
   moving" doesn't mean anything went wrong.
3. **Wait for `objectDetection() != Moving`** (`motionSettled()`) — the
   actual completion: either the gripper reached the requested
   position, or it stopped early on a detected object. A timeout here
   fails the move.

`command` itself is kept as one persistent `GripperCommand` across
both moves in `main()` (built once via `GripperCommand::defaults()`,
then mutated by each `moveTo()` call) rather than rebuilt from scratch
per move, since only `positionRequest` and the `GoTo` bit actually
change between them.
