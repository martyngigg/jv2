cmake_minimum_required(VERSION 3.5)

project(
  jv2
  VERSION 0.1
  LANGUAGES CXX)

set(DESCRIPTION "jv2")
set(AUTHOR "Team jv2")
set(VERSION_MAJOR "0")
set(VERSION_MINOR "1")
set(VERSION_PATCH "0")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_DEFAULT_MAJOR_VERSION 6)
find_package(OpenGL REQUIRED)
find_package(
  Qt6
  COMPONENTS Core
             Gui
             Widgets
             Network
             Charts
             OpenGL
             OpenGLWidgets
             REQUIRED)

find_package(Qt6OpenGL 6.0 REQUIRED)
find_package(Qt6OpenGLWidgets 6.0 REQUIRED)

set(PROJECT_SOURCES
    frontend/main.cpp
    frontend/mainwindow.cpp
    frontend/filtering.cpp
    frontend/datagathering.cpp
    frontend/searching.cpp
    frontend/nexusInteraction.cpp
    frontend/mainwindow.h
    frontend/mainwindow.ui
    frontend/httprequestworker.cpp
    frontend/httprequestworker.h
    frontend/jsontablemodel.cpp
    frontend/jsontablemodel.h)

if(WIN32)
  # Add global NOMINMAX define for Windows systems, and inhibit console creation
  add_definitions("-DNOMINMAX")
  set(GUI_TYPE WIN32)
  if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-mwindows")
  endif(MINGW)
endif(WIN32)
# Set bundle info
if(APPLE)
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
endif(APPLE)

# Resources
set(jv2_RES_QRC "frontend/icons.qrc")

add_executable(jv2 MACOSX_BUNDLE ${PROJECT_SOURCES} ${jv2_RES_QRC})

target_link_libraries(
  jv2 PRIVATE # External libs
              Qt6::Widgets Qt6::Core Qt6::Network Qt6::Charts OpenGL::GL)

set_target_properties(
  jv2
  PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

qt_finalize_executable(jv2)
