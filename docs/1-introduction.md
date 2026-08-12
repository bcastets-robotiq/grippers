# Introduction

A standalone, ROS-independent C++ SDK for controlling Robotiq 2F adaptive
grippers (2F-85 / 2F-140 / Hand-E class) over their Modbus RTU serial link.
Cross-platform: Linux, Windows, macOS. It also compiles for freestanding /
RTOS targets such as STM32 microcontrollers — see
[Embedded / bare-metal builds](embedded-stm32-builds.md).



## License

BSD-3-Clause. Portions derived from PickNik Robotics'
[ros2_robotiq_gripper](https://github.com/PickNikRobotics/ros2_robotiq_gripper)
driver (BSD-3-Clause); original copyright notices are preserved in the
affected files and full history is preserved in git.

<!-- docs-site:exclude -->
## Where to go next

- [How it works](how-it-works.md) — the threading model, the typed
  command/status API, and the five-step usage walkthrough.
- [Building and running the example](building-and-running.md) — compile
  the SDK and run the bundled example against a real gripper.
- [Using the SDK in your own project](using-the-sdk-in-your-project.md) —
  wire the SDK into your own application instead.
- [Embedded / bare-metal builds](embedded-stm32-builds.md) — compiling for
  STM32-class and other freestanding/RTOS targets.

<!-- /docs-site:exclude -->
