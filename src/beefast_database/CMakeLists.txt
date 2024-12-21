cmake_minimum_required(VERSION 3.8)
project(beefast_database)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(beefast_interfaces REQUIRED)

add_subdirectory(src/SQLiteCpp)
include_directories(
  src/SQLiteCpp/include
)

include_directories(
  include
)

set(dependencies
  rclcpp
  std_msgs
  sensor_msgs
  geometry_msgs
  beefast_interfaces
)

#dynamic lib
set(library_name ${PROJECT_NAME}_core)
add_library(${library_name} SHARED
  src/database_tool.cpp
  src/beefast_db.cpp
  src/database_server.cpp
)

ament_target_dependencies(${library_name}
  ${dependencies}
)
target_link_libraries(${library_name}
  sqlite3
  SQLiteCpp)

# exec
add_executable(${PROJECT_NAME}_node src/main.cpp)
target_link_libraries(${PROJECT_NAME}_node)
ament_target_dependencies(${PROJECT_NAME}_node ${dependencies})
target_link_libraries(${PROJECT_NAME}_node
  ${library_name}
)

set(executable_test testdb)
add_executable(${executable_test}
  src/testdb.cpp
)

ament_target_dependencies(${executable_test}
  ${dependencies}
)

target_link_libraries(${executable_test}
  ${library_name}
)

set(exe_createdb createdb)
add_executable(${exe_createdb}
  src/createdb.cpp
)
ament_target_dependencies(${exe_createdb}
  ${dependencies}
)
target_link_libraries(${exe_createdb}
  ${library_name}
)

set(exe_service testsrv)
add_executable(${exe_service}
  src/testdbserver.cpp
)
ament_target_dependencies(${exe_service}
  ${dependencies}
)
target_link_libraries(${exe_service}
  ${library_name}
)


install(TARGETS
  ${library_name}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS ${PROJECT_NAME}_node
        DESTINATION lib/${PROJECT_NAME})

install(TARGETS ${executable_test}
        DESTINATION lib/${PROJECT_NAME})

install(TARGETS ${exe_createdb}
        DESTINATION lib/${PROJECT_NAME})

install(TARGETS ${exe_service}
        DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY include/
  DESTINATION include/
)
ament_export_include_directories(include)
ament_export_libraries(${library_name})
ament_export_dependencies(${dependencies})

ament_package()
