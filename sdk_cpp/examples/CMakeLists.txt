add_executable(move_gripper move_gripper.cpp)
target_link_libraries(move_gripper PRIVATE grippers_sdk)

add_executable(quick_start quick_start.cpp)
target_link_libraries(quick_start PRIVATE grippers_sdk)

# Backs the header \snippet examples that aren't already covered by
# quick_start/move_gripper (see snippets.cpp). Compiled by every normal
# build, so a broken doc example fails the build like any other code;
# never run, so it has no real hardware dependency. Needs GRIPPERS_BUILD_FAKE
# for its makeFakeGripper() example — same default as examples themselves,
# so this only actually needs guarding against an explicit override.
if(GRIPPERS_BUILD_FAKE)
  add_executable(doc_snippets snippets.cpp)
  target_link_libraries(doc_snippets PRIVATE grippers_sdk)
endif()
