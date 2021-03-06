SET(CFLAGS -m64)
SET(CXXFLAGS -m64)

SET(OCULUS_PATH "D://SDK's//OculusSDK_1.10")
SET(SDL_PATH "D://SDL2")
set(OpenCV_FOUND 1)

project(Project_webcam_oculus)

SET(execName webcam_oculus)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 OLD)
	cmake_policy(SET CMP0015 OLD)  
endif(COMMAND cmake_policy)

SET(EXECUTABLE_OUTPUT_PATH ".")

if(MSVC12)
	SET(VS_VERSION VS2013)
else(MSVC12) 
	if(MSVC11)
		SET(VS_VERSION VS2012)
	else(MSVC11)
		if(MSVC10)
			SET(VS_VERSION VS2010)
		endif(MSVC10)
	endif(MSVC11)
endif(MSVC12)

if (CMAKE_CL_64) # 64 bits
	SET(OCULUS_LIBRARY_DIRS ${OCULUS_PATH}/LibOVR/Lib/Windows/x64/Release/${VS_VERSION})
	SET(SDL_LIBRARY_DIRS ${SDL_PATH}/lib/x64)
else(CMAKE_CL_64) # 32 bits
	SET(OCULUS_LIBRARY_DIRS ${OCULUS_PATH}/LibOVR/Lib/Windows/Win32/Release/${VS_VERSION})
	SET(SDL_LIBRARY_DIRS ${SDL_PATH}/lib/x86)
endif(CMAKE_CL_64)

SET(GLEW_LIBRARY_DIRS $ENV{GLEW_LIBRARY_DIRS})
SET(GLEW_LIBRARIES $ENV{GLEW_LIBRARY})
SET(GLEW_INCLUDE_DIRS $ENV{GLEW_INCLUDE_DIRS})

SET(GLUT_INCLUDE_DIRS $ENV{GLUT_INCLUDE_DIRS})
SET(GLUT_LIBRARY_DIRS $ENV{GLUT_LIBRARY_DIRS})
SET(GLUT_LIBRARY $ENV{GLUT_LIBRARY})

SET(OCULUS_INCLUDE_DIRS ${OCULUS_PATH}/LibOVR/Include ${OCULUS_PATH}/LibOVRKernel/Src)
SET(OCULUS_LIBRARIES LibOVR)

SET(SDL_INCLUDE_DIRS ${SDL_PATH}/include)
SET(SDL_LIBRARIES SDL2 SDL2main)


# Mandatory
find_package(CUDA 7.0 REQUIRED)
find_package(OpenGL 3.0 REQUIRED)

find_package(OpenCV COMPONENTS core highgui imgproc REQUIRED)

include_directories(${GLUT_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${OCULUS_INCLUDE_DIRS})
include_directories(${SDL_INCLUDE_DIRS})
include_directories(${CUDA_INCLUDE_DIRS})


link_directories(${GLUT_LIBRARY_DIRS})
link_directories(${GLEW_LIBRARY_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS})
link_directories(${SDL_LIBRARY_DIRS})
link_directories(${OCULUS_LIBRARY_DIRS})


SET(SRC_FOLDER src)
FILE(GLOB_RECURSE SRC_FILES "${SRC_FOLDER}/*.cpp")
 
ADD_EXECUTABLE(${execName} ${SRC_FILES})
set_property(TARGET ${execName} PROPERTY OUTPUT_NAME "webcam_oculus")
add_definitions(-std=c++0x)# -m32)

# Add the required libraries for linking:
TARGET_LINK_LIBRARIES(${execName}
			${OPENGL_LIBRARIES}
           		${GLUT_LIBRARY}
			${GLEW_LIBRARIES}			
			#${OpenCV_LIBRARIES}
			${SDL_LIBRARIES}
			${OCULUS_LIBRARIES}
 )

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3" ) # Release Perf mode