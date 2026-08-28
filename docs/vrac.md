#### About command structure
A command is composed of bytes:
- action
- position
- speed
- force

position, speed and force are unsigned int (uint8_t) coded on a full byte. Asigning a value is straitforward.

Example:
```cpp
command.position = 100
```

action is a byte composed of several bits which have a dedicated function. It is defined as a NamedBitArray.
The action byte is build using the set function of NamedBitArray class and the eNum ActionRequestBit which identify the different bits of the action byte.

Example:
```cpp
command.action.set(Robotiq::ActionRequestBit.GoTo,False) //set goto bit to true
Command.action.set(Robotiq::ActionRequestBit.AutoRelease) //Set autoRelease to false
```


#### About command structure

The gripper status is composed of the following bytes:
- gripperStatus
- faultStatus
- positionRequestEcho
- position
- current

positionRequestEcho, position and current are unsigned int (uint8_t) coded on a full byte. interpret their value is straitforward.

Example:
```cpp
std::cout << gripper.getStatus().position << std::endl;
```
This would print the current finger position in bit.

```bash
126
```
gripperStatus and faultStatus are bytes composed of several bits which have a dedicated function. They are defined as a NamedBitArray.
Information is retrieved using functions member of GripperStatus and FaultStatus.



Example 1: status information coded on 1 bit
```cpp
std::cout << gripper.getStatus().goToEnabled() << std::endl;
```
This would print goto bit value.

```bash
True
```

Example 2: status information coded on more than 1 bit

.gripperStatus.activationState() == ActivationState::Complete

```cpp
std::cout << gripper.getStatus().activationState () << std::endl;
```
```bash
Robotiq::ActivationState.Complete
```

This would print the eNum correspondong to the activationState. An assertion could be done on this enum.

```cpp
std::cout << gripper.getStatus().activationState () ==  Robotiq::ActivationState.Complete << std::endl;
```

```bash
True
```

---------------------------------------
---------------------------------------

```sh
cmake -S sdk_cpp -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build            # run unit tests, no hardware needed
```

Then run the example:

```sh
./build/examples/move_gripper /dev/ttyUSB0    # macOS: /dev/tty.usbserial-XXXX
```

It activates the gripper (calibration sweep), opens, and closes — keep
the jaws clear. See [Serial port notes](#serial-port-notes) below for
port permissions and latency-timer tuning.