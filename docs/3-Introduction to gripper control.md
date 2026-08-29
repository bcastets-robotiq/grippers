# Introduction to Gripper control

Robotiq grippers are controlled writting command and read status from its memory. The memory read and write is done using Modbus RTU.

## Command (Holding registers registers 1000 - 1003)

The registers use to command the gripper are composed of 3 registers of 16bits. Each register is splitted in 2 bytes(8bits) which makes a total of 6 bytes.

![Gripper holding registers' bytes](./_static/command_registers.png)

position, speed and force bytes are unsigned interger coded on 8 bits with a value in the range 0-255.

action byte is composed of several bits which dedicated function. The bits which constitute the action bytes are set using the set function. Bits are idenfied using the eNum Robotiq::ActionRequestBit.

## Status (Input registers 2000 - 2002)

The registers use to retrieve the status of the gripper are composed of 3 registers of 16bits. Each register is splitted in 2 bytes(8bits) which makes a total of 6 bytes.

![Gripper input registers' bytes](./_static/gripper_status_1.png)

![Gripper input registers' bytes](./_static/gripper_status_2.png)

positionRequestEcho, position and current bytes are unsigned interger coded on 8 bits with a value in the range 0-255.

gripperStatus and faultStatus bytes are composed of several bits which host specific information about the gripper status. Some information like gGTO are coded in 1 bit while others like gOBJ or gFLT are coded on several bits. Information coded on 1 bit are boolean while other are eNum.

## Control method

The communication flow to control the gripper is tipically the following:
- Build a command
- Send the command to the gripper
- Wait for the gripper to acknowledge the command
- Wait for the gripper to complete the action
- Check final status

```cpp
//Build command
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

# How the C++ driver hande the communication with the gripper

The c++ driver have been developed with the objective to maximize communication frequency.

The `Gripper` object owns a background thread that continuously exchanges
FC 0x17 Modbus (read&write) transaction with the gripper — up to ~200 Hz at 115200 baud. That thread is the only thing that directly communicate with the gripper.