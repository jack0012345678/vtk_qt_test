
cmake_minimum_required(VERSION 3.22)
project(interview_project)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/Qt5.12.8/5.12.8/msvc2017_64")
set(VTK_DIR "D:/VTK-8.2.0-Build")

find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})


include_directories(include)

add_executable(interview_project
    main.cpp
    src/MainWindow.cpp
    src/DICOMThread.cpp
    src/SliceInteract.cpp

    include/MainWindow.h
    include/DICOMThread.h
    include/SliceInteract.h

)


target_link_libraries(interview_project PRIVATE Qt5::Widgets ${VTK_LIBRARIES})
