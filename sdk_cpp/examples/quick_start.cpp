//! [qs-includes]
// Import gripper C++ driver
#include <Robotiq/gripper.hpp> // Gripper, GripperCommand/Status, activate(), recoverFromFault()

// Import utilities libraries
#include <iostream> // Library use to write message in the terminal.
#include <chrono>   // duration literals for the waitFor() timeouts below (1s, 200ms, ...)
using namespace std::chrono_literals; // enables the 1s / 200ms / 5s literals below
//! [qs-includes]

int main() {
    // 1- Create the connection config
    //! [qs-config]
    Robotiq::ConnectionConfig config;
    config.serial.port = "COM4"; //or "/dev/ttyUSB0" for linux. Adjust the port name according to your system.
    //! [qs-config]

    // 2- Create the gripper object
    //! [qs-create-gripper]
    Robotiq::Gripper gripper = Robotiq::Gripper(config);
    //! [qs-create-gripper]

    // 3- Activate the gripper
    //! [qs-activate]
    Robotiq::ActivationResult activationResult = Robotiq::activate(gripper);
    //! [qs-activate]

    // 4- Create a command to move the gripper
    //! [qs-create-command]
    Robotiq::GripperCommand command = Robotiq::GripperCommand::defaults();
    command.action.set(Robotiq::ActionRequestBit::GoTo);
    command.positionRequest = 100;
    command.speed = 255;
    command.force = 255;
    //! [qs-create-command]

    // 5- Set command
    //! [qs-send-command]
    gripper.setCommand(command);
    //! [qs-send-command]

    //! [qs-wait]
    // 6- Wait for the gripper to echo
    bool positionRequestEchoed = Robotiq::waitFor([&]{return gripper.getStatus().positionRequestEcho == command.positionRequest;},1s);

    // 7- Wait for the action to complete
    bool motionCompleted = Robotiq::waitFor([&]{return (gripper.getStatus().gripperStatus.objectDetection() != Robotiq::ObjectDetection::Moving);},10s);
    //! [qs-wait]

    //! [qs-status]
    // 8- retrieve status
    uint8_t currentPosition = gripper.getStatus().position;

    // Print retrieved status
    std::cout << "Current position : " << static_cast<int>(currentPosition) << std::endl;
    //! [qs-status]

    // End of the program
    return 0;
}