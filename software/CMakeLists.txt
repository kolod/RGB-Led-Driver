cmake_minimum_required(VERSION 3.12)
project(rgb-driver-gui VERSION 0.1.0 LANGUAGES CXX)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR
		"Prevented in-tree built. "
		"Please create a build directory outside of the source code and call cmake from there. "
		"Thank you."
	)
endif()

include(CTest)
enable_testing()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Widgets Svg SerialBus SerialPort LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Svg SerialBus SerialPort LinguistTools)

set(TS_FILES
	rgb-driver-gui_ua.ts
	rgb-driver-gui_ru.ts
	rgb-driver-gui_de.ts
)

set(QRC_FILES
	resources/resources.qrc
)

set(PROJECT_SOURCES
	main.cpp
	mainwindow.cpp
	mainwindow.h
	mainwindow.ui
	colorlistmodel.h
	colorlistmodel.cpp
	${TS_FILES}
)

add_subdirectory(QGlobalShortcut)

if(WIN32)
	list(APPEND PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/resources.rc")
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
	qt_add_binary_resources(resources ${QRC_FILES})
	qt_add_resources(PROJECT_SOURCES ${QRC_FILES})
	qt_add_executable(rgb-driver-gui WIN32 MANUAL_FINALIZATION ${PROJECT_SOURCES})
	qt_add_translations(rgb-driver-gui TS_FILES ${TS_FILES} RESOURCE_PREFIX "i18n" OUTPUT_TARGETS resources)
else()
	qt5_add_binary_resources(resources ${QRC_FILES})
	qt5_add_resources(PROJECT_SOURCES ${QRC_FILES})
	if(ANDROID)
		add_library(rgb-driver-gui SHARED ${PROJECT_SOURCES})
	else()
		add_executable(rgb-driver-gui WIN32 ${PROJECT_SOURCES})
	endif()
	qt5_add_translations(rgb-driver-gui TS_FILES ${TS_FILES} RESOURCE_PREFIX "i18n" OUTPUT_TARGETS resources)
endif()

add_dependencies(rgb-driver-gui resources)
add_dependencies(rgb-driver-gui qglobalshortcut)

target_include_directories(rgb-driver-gui PRIVATE QGlobalShortcut)
target_link_libraries(rgb-driver-gui PRIVATE qglobalshortcut)
target_link_libraries(rgb-driver-gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(rgb-driver-gui PRIVATE Qt${QT_VERSION_MAJOR}::Svg)
target_link_libraries(rgb-driver-gui PRIVATE Qt${QT_VERSION_MAJOR}::SerialBus)
target_link_libraries(rgb-driver-gui PRIVATE Qt${QT_VERSION_MAJOR}::SerialPort)

if(QT_VERSION_MAJOR EQUAL 6)
	qt_finalize_executable(rgb-driver-gui)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
