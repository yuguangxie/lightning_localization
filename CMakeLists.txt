cmake_minimum_required(VERSION 3.16)

project(lightning_localization)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

set(CMAKE_BUILD_TYPE Release)

add_definitions("-DPCL_NO_PRECOMPILE")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fopenmp -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -fopenmp -fPIC -g -ggdb")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -g -fopenmp -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -g")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(cmake/packages.cmake)

rosidl_generate_interfaces(${PROJECT_NAME}
  "srv/LocCmd.srv"
  "srv/SaveMap.srv"
  "srv/SetInitialPose.srv"
  DEPENDENCIES std_msgs geometry_msgs sensor_msgs nav_msgs
)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp
)

install(DIRECTORY config launch docs scripts rviz
  DESTINATION share/${PROJECT_NAME}
)

add_subdirectory(thirdparty/livox_ros_driver)
add_subdirectory(src)

ament_export_dependencies(rosidl_default_runtime)

ament_package()
