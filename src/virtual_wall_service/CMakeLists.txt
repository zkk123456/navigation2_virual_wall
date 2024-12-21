cmake_minimum_required(VERSION 3.8)
project(virtual_wall_service)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(map_msgs REQUIRED)
find_package(nav2_costmap_2d REQUIRED)
find_package(nav2_util REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(beefast_filter_mask REQUIRED)
find_package(nav2_map_server REQUIRED)
find_package(beefast_interfaces REQUIRED)

include_directories(
  include
)

set(dependencies
  rclcpp
  std_msgs
  sensor_msgs
  visualization_msgs
  nav2_msgs
  geometry_msgs
  map_msgs
  nav2_costmap_2d
  nav2_util
  rclcpp_lifecycle
  beefast_filter_mask
  nav2_map_server
  beefast_interfaces
)

add_executable(virtual_wall_server 
  src/virtual_wall_server.cpp
  src/main.cpp
)
ament_target_dependencies(virtual_wall_server ${dependencies})


install(TARGETS virtual_wall_server
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)


#dynamic lib
set(library_name ${PROJECT_NAME}_core)
add_library(${library_name} SHARED
  src/virtual_wall_server.cpp
)

ament_target_dependencies(${library_name}
  ${dependencies}
)

target_link_libraries(virtual_wall_server
  ${library_name}
  rclcpp::rclcpp
)

install(TARGETS
  ${library_name}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include/
)

ament_export_include_directories(include)
ament_export_libraries(${library_name})
ament_export_dependencies(${dependencies})
ament_package()
