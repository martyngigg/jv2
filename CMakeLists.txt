cmake_minimum_required(VERSION 3.5)

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
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")

# Setup Conan for dependency management
include(conan-jv2)

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
    frontend/jsontablemodel.h
    frontend/chartview.cpp
    frontend/chartview.h
    frontend/graphwidget.cpp
    frontend/graphwidget.h
    frontend/graphwidget.ui
    frontend/mysortfilterproxymodel.cpp
    frontend/mysortfilterproxymodel.h)

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
  list(APPEND PROJECT_SOURCES frontend/platforminit.cpp)
endif()

# Resources
set(jv2_RES_QRC "frontend/icons.qrc")

qt6_add_executable(jv2 MACOSX_BUNDLE ${PROJECT_SOURCES} ${jv2_RES_QRC})
target_link_libraries(jv2
  PRIVATE
    Qt6::Core Qt6::Widgets  Qt6::Network Qt6::Charts Qt6::Xml Qt6::OpenGLWidgets
    Qt6::QXcbIntegrationPlugin Qt6::XcbQpaPrivate
)

set_target_properties(
  jv2
  PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)
