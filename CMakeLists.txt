cmake_minimum_required(VERSION 3.10)

# set the project name
project(modbusctl)

set(CMAKE_BUILD_TYPE Debug)

find_package(PkgConfig REQUIRED)

pkg_check_modules(MODBUS REQUIRED libmodbus)


# add the executable
add_executable(modbusctl
	src/main.c
	src/fconf.c
)


target_link_directories(modbusctl
	PUBLIC ${MODBUS_LIBRARY_DIRS}
)

target_include_directories(modbusctl AFTER
	PUBLIC ${MODBUS_INCLUDE_DIRS}
)

target_link_libraries(modbusctl
	${MODBUS_LIBRARIES}
)
