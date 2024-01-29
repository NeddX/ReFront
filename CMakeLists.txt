cmake_minimum_required(VERSION 3.20)

project("CMM")

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
include_directories("include/")

# Create a custom build command and a dummy target for it to copy
# compile_commands.json to the source directory for LSPs.
add_custom_command(
  OUTPUT ${CMAKE_SOURCE_DIR}/compile_commands.json
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/compile_commands.json
          ${CMAKE_SOURCE_DIR}/compile_commands.json
  DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
  COMMENT "Copying compile_commands.json...")

add_custom_target(copy_compile_commands ALL
                  DEPENDS ${CMAKE_SOURCE_DIR}/compile_commands.json)

add_subdirectory("CMCompiler")
