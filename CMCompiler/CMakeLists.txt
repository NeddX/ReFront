project("CMCompiler")

file(GLOB_RECURSE CMC_SRC_FILES "src/*.cpp")
file(GLOB_RECURSE CMC_HDR_FILES "src/*.h")

add_executable(cmc ${CMC_SRC_FILES} ${CMC_HDR_FILES})

target_include_directories(cmc PRIVATE "include/")
