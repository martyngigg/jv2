cmake_minimum_required(VERSION 3.15)

project(
  jv2
  VERSION 0.1
  LANGUAGES CXX)

# Metadata
set(DESCRIPTION "jv2")
set(AUTHOR "Team jv2")
set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION_PATCH "0")

# Global CMake settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")

# Configure options
option(JV2_USE_CONAN "If True, use the Conan package manager" OFF)

# Setup Conan for dependency management
if(JV2_USE_CONAN)
  include(conan-jv2)
endif()

find_package(
  Qt6
  COMPONENTS Core
             Gui
             Widgets
             Network
             Charts
             Xml
             OpenGL
             OpenGLWidgets
  REQUIRED)

# Common sources
set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/filtering.cpp
    src/datagathering.cpp
    src/searching.cpp
    src/nexusInteraction.cpp
    src/mainwindow.h
    src/mainwindow.ui
    src/httprequestworker.cpp
    src/httprequestworker.h
    src/jsontablemodel.cpp
    src/jsontablemodel.h
    src/chartview.cpp
    src/chartview.h
    src/graphwidget.cpp
    src/graphwidget.h
    src/graphwidget.ui
    src/mysortfilterproxymodel.cpp
    src/mysortfilterproxymodel.h)

# OS-dependent flags
if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE
      "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
elseif()
  set(MACOSX_BUNDLE_ICON_FILE "jv2.icns")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING
      "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
  set(MACOSX_BUNDLE_BUNDLE_NAME "jv2")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION
      "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
  set(MACOSX_BUNDLE_COPYRIGHT "${AUTHOR}")

  # Set some specific C++11 related options here (set_property below does not
  # seem to persist)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

  # Suppress warnings for undefined template vars
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-undefined-var-template")

  set(CMAKE_OSX_ARCHITECTURES "x86_64")
  add_definitions(-D_MAC)
  find_package(GLUT REQUIRED)
else()
  if(JV2_USE_CONAN)
    list(APPEND PROJECT_SOURCES src/platforminit.cpp)
    set(STATIC_PLATFORM_PLUGINS Qt6::QXcbIntegrationPlugin Qt6::XcbQpaPrivate)
  endif()
endif()

# Resources
set(jv2_RES_QRC "src/icons.qrc")

qt6_add_executable(jv2 MACOSX_BUNDLE ${PROJECT_SOURCES} ${jv2_RES_QRC})
target_include_directories(jv2 PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(
  jv2
  PRIVATE Qt6::Core
          Qt6::Widgets
          Qt6::Network
          Qt6::Charts
          Qt6::Xml
          Qt6::OpenGLWidgets
          ${STATIC_PLATFORM_PLUGINS})

set_target_properties(
  jv2
  PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
