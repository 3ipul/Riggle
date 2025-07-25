# Riggle/CmakeLists.txt

cmake_minimum_required(VERSION 3.15)
project(Riggle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static linking configuration
set(BUILD_SHARED_LIBS OFF)
add_definitions(-DSFML_STATIC)  # Global SFML static definition

# Set paths
set(SFML_ROOT "${CMAKE_SOURCE_DIR}/external/SFML")
set(SFML_INCLUDE_DIR "${SFML_ROOT}/include")
set(SFML_LIB_DIR "${SFML_ROOT}/lib")

# Add subdirectories
# add_subdirectory(common)
add_subdirectory(Riggle_Core)
add_subdirectory(Riggle_Editor)