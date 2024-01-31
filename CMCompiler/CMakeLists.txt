project("CMCompiler")

file(GLOB_RECURSE CMC_SRC_FILES "src/*.cpp")
file(GLOB_RECURSE CMC_HDR_FILES "src/*.h")

add_executable(cmc ${CMC_SRC_FILES} ${CMC_HDR_FILES})

set_property(TARGET cmc PROPERTY CXX_STANDARD 20)

target_include_directories(cmc PRIVATE "include/")

add_subdirectory("vendor/ALVM/alvm" ${CMAKE_BINARY_DIR}/alvm)
set(ALVM_INCLUDE_DIRS "vendor/ALVM/alvm/include")
set(ALVM_LIBRARIES alvm-static)
target_link_libraries(cmc ${ALVM_LIBRARIES})
target_include_directories(cmc PRIVATE ${ALVM_INCLUDE_DIRS})

add_subdirectory("vendor/fmt" ${CMAKE_BINARY_DIR}/fmt)
set(FMT_INCLUDE_DIRS "vendor/fmt/include")
set(FMT_LIBRARIES fmt)
target_link_libraries(cmc ${FMT_LIBRARIES})
target_include_directories(cmc PRIVATE ${FMT_INCLUDE_DIRS})

add_subdirectory("vendor/nlohmann_json" ${CMAKE_BINARY_DIR}/nlohmann_json)
set(NLOHMANN_JSON_INCLUDE_DIRS "vendor/nlohmann_json/include")
set(NLOHMANN_JSON_LIBRARIES nlohmann_json)
target_link_libraries(cmc ${NLOHMANN_JSON_LIBRARIES})
target_include_directories(cmc PRIVATE ${NLOHMANN_JSON_INCLUDE_DIRS})
