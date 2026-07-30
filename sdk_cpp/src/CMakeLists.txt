target_sources(grippers_sdk PRIVATE
  gripper.cpp
  frequency.hpp
  exchange_period.hpp
  logger.cpp
  detail/gripper_modbus_client.cpp
)

if(GRIPPERS_BUILD_FAKE)
  target_sources(grippers_sdk PRIVATE
    fake/gripper_factory.cpp
    fake/gripper_serial.hpp
    fake/gripper_serial.cpp
    fake/gripper_server.hpp
    fake/gripper_server.cpp
    fake/register_model.hpp
    fake/register_model.cpp
    fake/status_writer.hpp
    fake/status_writer.cpp
  )
endif()

# libserialport-backed transport: desktop only (see GRIPPERS_BUILD_DEFAULT_SERIAL).
if(GRIPPERS_BUILD_DEFAULT_SERIAL)
  target_sources(grippers_sdk PRIVATE detail/default_serial.cpp)
endif()
