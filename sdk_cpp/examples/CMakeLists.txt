add_executable(move_gripper move_gripper.cpp)
target_link_libraries(move_gripper PRIVATE grippers_sdk)

add_executable(quick_start quick_start.cpp)
target_link_libraries(quick_start PRIVATE grippers_sdk)
