project(lab3)
cmake_minimum_required(VERSION 2.8)
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

file(GLOB SOURCES src/lab3.c)
file(GLOB SOURCES_SEQ src/lab3_seq.c)
file(GLOB SOURCES_MPI src/lab3_mpi.c)


add_executable(${PROJECT_NAME} ${SOURCES})
add_executable(${PROJECT_NAME}_seq ${SOURCES_SEQ})
add_executable(${PROJECT_NAME}_mpi ${SOURCES_MPI})
target_link_libraries(${PROJECT_NAME}_mpi m)
SET(CMAKE_C_COMPILER mpicc)

if(MPI_COMPILE_FLAGS)
  set_target_properties(${PROJECT_NAME}_mpi PROPERTIES
    COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
  set_target_properties(${PROJECT_NAME}_mpi PROPERTIES
    LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()


set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)
set_property(TARGET ${PROJECT_NAME}_seq PROPERTY C_STANDARD 99)
set_property(TARGET ${PROJECT_NAME}_mpi PROPERTY C_STANDARD 99)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
