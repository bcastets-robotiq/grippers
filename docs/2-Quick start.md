# Quick start

This is a minimalist introduction showing how to control the gripper using the C++ driver.
Refer to API documentation to get more details.

> **Note :**
> This example does not handle potential errors. Refer to move_gripper.cpp for a robust example.

## Import dependencies

```cpp
// Import gripper C++ driver
#include <Robotiq/gripper.hpp>

// Import utilities libraries
#include <iostream> // Library use to write message in the terminal.
#include <chrono>   // duration literals 1s, 200ms, ...
using namespace std::chrono_literals; // enables the 1s / 200ms / 5s literals
```

## Write the main program

```cpp
int main(){
    //The code we are going to write from now on
}
```

### Create a connection configuration
Create a ConnectionConfig object and specify which port the gripper is connected to.
This example supposes that the gripper is connected to COM4 of a Windows PC. Adjust the code to fit the port you are using to connect the gripper.

```cpp
Robotiq::ConnectionConfig config;
config.serial.port = "COM4"; //or "/dev/ttyUSB0" for linux
```

### Create a gripper object
Create a gripper object using the previously created connection configuration.

```cpp
Robotiq::Gripper gripper = Robotiq::Gripper(config);
```

### Activate the gripper
Gripper activation is the first action to perform before being able to use the gripper. The C++ driver provide a function to perform gripper activation.

The activate function is marked `[[nodiscard]]`, which means the result must be saved into a variable, it cannot be discarded.

```cpp
Robotiq::ActivationResult activationResult = Robotiq::activate(gripper);
```

> **Note :**
> If the gripper is already activated, the activate function does nothing. To force the activation process the gripper activate (rACT) bit has to be set to false or the gripper power has to be removed.
>
> This is different from `recoverFromFault()`, which also reactivates the gripper immediately (running the calibration sweep) as part of the same call — use the approach above instead if you want the gripper to stay deactivated until you call `activate()` yourself.

### Create a command and send it

To control the gripper you have to write a command with appropriate parameter and send it.

The command is initially built from a default command.
The GoTo bit of the action register has to be set to 1 so that the gripper moves to the position written in its position register.
The desired position, speed and force have to be defined.

```cpp
Robotiq::GripperCommand command = Robotiq::GripperCommand::defaults();
command.action.set(Robotiq::ActionRequestBit::GoTo);
command.positionRequest = 100;
command.speed = 255;
command.force = 255;
```

### Send the command
Once the command is prepared we can send it to the gripper using the setCommand function.

```cpp
gripper.setCommand(command);
```

### Wait for the action to be completed
The C++ driver comes with a convenient wait function that can be used to wait for a gripper action to complete before moving to the next step of the program.

The wait function is marked `[[nodiscard]]`, which means the result must be saved into a variable, it cannot be discarded.

First we have to wait for the gripper to acknowledge the reception of the
command. Then we can wait for the command to complete.

```cpp
// Wait for the gripper to acknowledge the reception of the command
bool positionRequestEchoed = Robotiq::waitFor([&]{return gripper.getStatus().positionRequestEcho == command.positionRequest;},1s);

// Wait for the gripper to complete the move
bool settled = Robotiq::waitFor([&]{return (gripper.getStatus().gripperStatus.objectDetection() != Robotiq::ObjectDetection::Moving);},10s);
```

### Retrieve gripper status

The status of the gripper can be retrieved with the getStatus function. Here is an example where we retrieve and print the current position of the gripper.

```cpp
    // Retrieve gripper current position
    uint8_t currentPosition = gripper.getStatus().position;

    // Print gripper current position in terminal
    std::cout << "Current position : " << static_cast<int>(currentPosition) << std::endl;
```