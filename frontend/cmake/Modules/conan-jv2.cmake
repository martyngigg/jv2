# Fetch and include Conan-cmake integration if it doesn't exist
if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/master/conan.cmake" "${CMAKE_BINARY_DIR}/conan.cmake")
endif ()
include(${CMAKE_BINARY_DIR}/conan.cmake)

# Setup a conanfile.txt with dependency information and fold it into the
# build targets
conan_cmake_run(
  REQUIRES
    qt/6.3.1
    fontconfig/2.13.93
    glib/2.73.1
    openssl/1.1.1q
  BASIC_SETUP CMAKE_TARGETS
  GENERATORS cmake_find_package cmake_paths
  BUILD missing
  OPTIONS
    qt:shared=False qt:qtcharts=True
    glib:shared=False
    fontconfig:shared=False
)
include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
