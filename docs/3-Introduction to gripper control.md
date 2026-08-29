# Introduction to Gripper control

Robotiq grippers are controlled by writing commands to, and reading status from, their memory. The memory read and write is done using Modbus RTU.

## Command (Holding registers 1000 - 1002)

The registers used to command the gripper are composed of 3 registers of 16 bits. Each register is split into 2 bytes (8 bits), for a total of 6 bytes.

![Gripper holding registers' bytes](./_static/command_registers.png)

The positionRequest, speed and force bytes are unsigned integers coded on 8 bits, with a value in the range 0-255.

The action byte is composed of several bits, each with a dedicated function. The bits that make up the action byte are set using the `set` function. Bits are identified using the enum `Robotiq::ActionRequestBit`.

## Status (Input registers 2000 - 2002)

The registers used to retrieve the status of the gripper are composed of 3 registers of 16 bits. Each register is split into 2 bytes (8 bits), for a total of 6 bytes.

![Gripper input registers' bytes](./_static/gripper_status_1.png)

![Gripper input registers' bytes](./_static/gripper_status_2.png)

The positionRequestEcho, position and current bytes are unsigned integers coded on 8 bits, with a value in the range 0-255.

The gripperStatus and faultStatus bytes are composed of several bits that each hold specific information about the gripper status. Some information, like gGTO, is coded on 1 bit, while others, like gOBJ or gFLT, are coded on several bits. Information coded on 1 bit is boolean, while multi-bit information decodes to an enum.

## Control method

The communication flow to control the gripper is typically the following:
- Build a command
- Send the command to the gripper
- Wait for the gripper to acknowledge the command
- Wait for the gripper to complete the action
- Check final status

```cpp
// Build command
Robotiq::GripperCommand command = Robotiq::GripperCommand::defaults();
command.action.set(Robotiq::ActionRequestBit::GoTo);
command.positionRequest = 100;
command.speed = 255;
command.force = 255;

// Send command
gripper.setCommand(command);

// Wait for acknowledge
bool positionRequestEchoed = Robotiq::waitFor([&]{return gripper.getStatus().positionRequestEcho == command.positionRequest;},1s);

// Wait for action to complete
bool motionCompleted = Robotiq::waitFor([&]{return (gripper.getStatus().gripperStatus.objectDetection() != Robotiq::ObjectDetection::Moving);},10s);

// Check final status
uint8_t currentPosition = gripper.getStatus().position;
```

## How the C++ driver handles communication with the gripper

The C++ driver has been developed with the objective of maximizing communication frequency.

The `Gripper` object owns a background thread that continuously exchanges
FC 0x17 Modbus (read&write) transactions with the gripper — up to ~200 Hz at 115200 baud. That thread is the only thing that directly communicates with the gripper.