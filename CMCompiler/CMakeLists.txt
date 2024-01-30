project("CMCompiler")

file(GLOB_RECURSE CMC_SRC_FILES "src/*.cpp")
file(GLOB_RECURSE CMC_HDR_FILES "src/*.h")

add_executable(cmc ${CMC_SRC_FILES} ${CMC_HDR_FILES})

target_include_directories(cmc PRIVATE "include/")

add_subdirectory("vendor/ALVM" ${CMAKE_BINARY_DIR}ALVM/)
set(ALVM_INCLUDE_DIRS "vendor/ALVM/alvm/include")
set(ALVM_LIBRARIES alvm-static)
target_link_libraries(cmc ${ALVM_LIBRARIES})
target_include_directories(cmc PRIVATE ${ALVM_INCLUDE_DIRS})
