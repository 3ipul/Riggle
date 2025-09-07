# Riggle_Core/CMakeLists.txt

file(GLOB_RECURSE CORE_SRC "src/*.cpp")

add_library(Riggle_Core STATIC ${CORE_SRC})

target_include_directories(Riggle_Core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)