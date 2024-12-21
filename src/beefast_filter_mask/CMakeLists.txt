cmake_minimum_required(VERSION 3.8)
project(beefast_filter_mask)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(nav2_costmap_2d REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(map_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)

include_directories(
  include
)

set(dependencies
  rclcpp
  nav2_costmap_2d
  nav_msgs
  map_msgs
  sensor_msgs
  visualization_msgs
)

# exec
add_executable(virtual_wall_node 
  src/costmap.cpp
  src/virtual_wall.cpp
  src/main.cpp
)
ament_target_dependencies(virtual_wall_node ${dependencies})
install(TARGETS virtual_wall_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

#dynamic lib
set(library_name ${PROJECT_NAME}_core)
add_library(${library_name} SHARED
  src/costmap.cpp
  src/virtual_wall.cpp
)

ament_target_dependencies(${library_name}
  ${dependencies}
)

target_link_libraries(virtual_wall_node
  ${library_name}
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
