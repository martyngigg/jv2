cmake_minimum_required(VERSION 3.5)

project(
  backEndTest
  VERSION 0.1
  LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to
# qmake Android variables. Check https://doc.qt.io/qt/deployment-android.html
# for more information. They need to be set before the find_package( ...) calls
# below.

# if(ANDROID) set(ANDROID_PACKAGE_SOURCE_DIR
# "${CMAKE_CURRENT_SOURCE_DIR}/android") if (ANDROID_ABI STREQUAL "armeabi-v7a")
# set(ANDROID_EXTRA_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
# ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so) endif() endif()

set(QT_DEFAULT_MAJOR_VERSION 6)
find_package(OpenGL REQUIRED)
find_package(
  Qt6
  COMPONENTS Core
             Gui
             Widgets
             Network
             OpenGL
             OpenGLWidgets
             REQUIRED)

find_package(Qt6OpenGL 6.0 REQUIRED)
find_package(Qt6OpenGLWidgets 6.0 REQUIRED)

set(PROJECT_SOURCES
    frontend/main.cpp
    frontend/mainwindow.cpp
    frontend/mainwindow.h
    frontend/mainwindow.ui
    frontend/httprequestworker.cpp
    frontend/httprequestworker.h
    frontend/jsontablemodel.cpp
    frontend/jsontablemodel.h)

qt_add_executable(backEndTest MANUAL_FINALIZATION ${PROJECT_SOURCES})

target_link_libraries(
  backEndTest PRIVATE # External libs
                      Qt6::Widgets Qt6::Core Qt6::Network OpenGL::GL)

set_target_properties(
  backEndTest
  PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

if(QT_VERSION_MAJOR EQUAL 6)
  qt_finalize_executable(backEndTest)
endif()
