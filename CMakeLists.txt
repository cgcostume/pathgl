cmake_minimum_required(VERSION 2.8)
project(pathgl C CXX)
find_package(OpenGL REQUIRED)

find_path(GLM_INCLUDE_DIR glm/glm.hpp
    $ENV{GLM_HOME}
    /usr/include
    /usr/local/include
    /sw/include
    /opt/local/include
    DOC "The directory where glm/glm.hpp resides.")

find_path(FREEGLUT_INCLUDE_DIR GL/freeglut.h GL/glut.h
    $ENV{GLUT_HOME}/include
    $ENV{FREEGLUT_HOME}/include
    $ENV{PROGRAMFILES}/FREEGLUT/include
    /usr/include
    /usr/local/include
    /sw/include
    /opt/local/include
    DOC "The directory where GL/freeglut.h resides")
find_library(FREEGLUT_LIBRARY
	NAMES FREEGLUT freeglut glut
	PATHS
	$ENV{GLUT_HOME}/lib
	$ENV{FREEGLUT_HOME}/lib
	/usr/lib64
	/usr/lib
	/usr/local/lib64
	/usr/local/lib
	/sw/lib
	/opt/local/lib
	DOC "The FREEGLUT library")

find_path(GLEW_INCLUDE_DIR GL/glew.h
    $ENV{GLEW_HOME}/include
    $ENV{PROGRAMFILES}/GLEW/include
    /usr/include
    /usr/local/include
    /sw/include
    /opt/local/include
    DOC "The directory where GL/glew.h resides")
find_library(GLEW_LIBRARY
    NAMES GLEW glew glew32 glew32s
    PATHS
	$ENV{GLEW_HOME}/lib
    $ENV{PROGRAMFILES}/GLEW/lib
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
    /sw/lib
    /opt/local/lib
    DOC "The GLEW library")

include_directories(${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIR} ${FREEGLUT_INCLUDE_DIR} ${GLM_INCLUDE_DIR})
add_executable(pathgl pathgl.cpp trace.vert trace.frag)
target_link_libraries(pathgl ${OPENGL_LIBRARIES} ${GLEW_LIBRARY} ${FREEGLUT_LIBRARY})