cmake_minimum_required(VERSION 3.2.0)
# required to enable MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)
# set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)

project(ExtIO_RTL)

### build options
# default build type: Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE} " - Version: " ${VERSION} " / " ${LIBVER})

if (NOT MSVC)
    message(ERROR "CMakeLists.txt does only support the MSVC compiler")
endif()


# allow overriding cmake options with standard variables - from a project including this one
cmake_policy(SET CMP0077 NEW)  # set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(PTHREADS4W_VERSION 3)
add_subdirectory(pthread-win32)
if (TARGET libpthreadVC3)
    set( THREADS_FOUND  TRUE)
    set( Threads_FOUND  TRUE)
    set( THREADS_PTHREADS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pthread-win32" )
    set( THREADS_PTHREADS_WIN32_LIBRARY libpthreadVC3)
    set( CMAKE_THREAD_LIBS_INIT  libpthreadVC3)
else()
    message(FATAL_ERROR "ExtIO_RTL: target libpthreadVC3 not defined!")
endif()

add_subdirectory(libusb)
if (TARGET libusb_static)
    set( LIBUSB_FOUND TRUE )
    set( LIBUSB_LIBRARIES  libusb_static )
endif()

add_subdirectory(librtlsdr)


add_library(ExtIO_RTL SHARED
    src/control_tcp.cpp
    src/control.h
    src/ExtIO_RTL.cpp
    src/ExtIO_RTL.h
    src/ExtIO_RTL.rc
    src/LC_ExtIO_Types.h
    src/config_file.cpp
    src/config_file.h
    src/dllmain.cpp
    src/ExtIO_RTL.def
    src/resource.h
    src/targetver.h
    src/tuners.h
    src/tuners.cpp
    src/rates.h
    src/rates.cpp
    src/gui_dlg.cpp
    src/gui_dlg.h
)

set_target_properties(ExtIO_RTL PROPERTIES PREFIX "")
set_property(TARGET ExtIO_RTL PROPERTY CXX_STANDARD 17)
set_property(TARGET ExtIO_RTL PROPERTY CXX_STANDARD_REQUIRED ON)
target_compile_definitions(ExtIO_RTL PUBLIC _CRT_SECURE_NO_WARNINGS)
target_compile_definitions(ExtIO_RTL PRIVATE rtlsdr_STATIC)


# set our and override dependent libraries' MSVC_RUNTIME_LIBRARY
set_property(TARGET ExtIO_RTL PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET libpthreadVC3 PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET libusb_static PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(TARGET rtlsdr_static PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_include_directories(ExtIO_RTL PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    tomlplusplus/include
    librtlsdr/include
)

target_link_libraries(ExtIO_RTL PRIVATE
    rtlsdr_static
    libpthreadVC3
    libusb_static
)
