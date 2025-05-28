include_directories("smallpp")

add_executable(example "example/main.cpp")
target_include_directories(example PRIVATE "../res")

if(WIN32)
    file(GLOB_RECURSE SOURCES "generator/*.h" "generator/*.cpp")
    add_executable(generator ${SOURCES})

    target_include_directories(generator PRIVATE "generator")
endif()
