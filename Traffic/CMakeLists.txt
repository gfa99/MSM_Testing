# 1.CMake 最低的版本要求
cmake_minimum_required(VERSION 3.0.0)

# 2.设置（cmake 内置的）项目名称变量 PROJECT_NAME 其默认值为 demo
set(PROJECT_NAME demo)
set(PROJECT_VERSION 1.2.3.4)
# 3.项目名称以及版本（步骤2和3可以合在一起，直接写为 project(demo VERSION 0.1.0) ）
project(${PROJECT_NAME})


# 设置编译模式
set(CMAKE_BUILD_TYPE Release)

# 分别设置Debug和Release输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release/bin)


# 配置编译选项（add_compile_options命令添加的编译选项是针对所有编译器的(包括c和c++编译器)，而set命令设置CMAKE_C_FLAGS或CMAKE_CXX_FLAGS变量则是分别只针对c和c++编译器）
#add_compile_options(-g -Wall -std=c++11)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-Wall")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3")


# 判断操作系统及处理器架构
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(OS_TYPE "OS_WINDOWS")
elseif(CMAKE_SYSTEM_NAME MATCHES "Mac")
    set(OS_TYPE "OS_MAC")
else()
    set(OS_TYPE "OS_LINUX")
endif(CMAKE_SYSTEM_NAME MATCHES "Windows")

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
	set(ARCH_TYPE "x86_64")
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
	set(ARCH_TYPE "aarch64")
else ()
	message(FATAL_ERROR "unexpected CMAKE_SYSTEM_PROCESSOR...")
endif()
message(STATUS "current os: ${OS_TYPE}, platform arch: ${ARCH_TYPE}")


# 设置宏定义
add_definitions(
	-D${OS_TYPE} 
	-D${ARCH_TYPE}
)


# 包含头文件目录（这是一个全局包含，向下传递。就是说如果某个目录的 CMakeLists.txt 中使用了该语句，其下所有的子目录默认也包含了对应的目录）
# 1) include_directories(x y):
# 影响范围更大 — 影响目录范围: 此CMakeList中的所有目标以及在其调用点之后添加的所有子目录中的目标都将路径 x y 添加到其include路径。
# 它相当于g++选项中的-I参数的作用，也相当于环境变量中增加路径到CPLUS_INCLUDE_PATH变量的作用。
# 2) target_include_directories(t x y):
# 影响范围可控 — 添加的 x y 只作用到目标的包含路径。一般地, 需要include某个库的时候, 使用这个命令来include库所在的路径。
# 它相当于g++命令的-L选项的作用，也相当于环境变量中增加LD_LIBRARY_PATH的路径的作用
# 3) 如果所有目标都使用相关的包含目录，则需要前者；果某个路径只是特定于某个目标，或者您想要更好地控制路径的可见性，则需要后者。target_include_directories 支持 PRIVATE PUBLIC 和 INTERFACE 限定符。
include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
)


# 配置需要编译的源文件
# 1）查找指定目录（. 代表当前目录）下的所有源文件，并将名称保存到 SROUCES_LIST 变量
#aux_source_directory(. SROUCES_LIST)
# 2）递归枚举对应目录下的的指定后缀文件（若是不用递归寻找，则 GLOB_RECURSE => GLOB）
#file(GLOB_RECURSE SROUCES_LIST 
#	${PROJECT_SOURCE_DIR}/src/*.c 
#	${PROJECT_SOURCE_DIR}/src/*.cpp
#)
# 3) 手动指定需要编译的源文件
set(SROUCES_LIST 
	main.cpp
	TrafficLight.cpp
)


# 指定编译生成的目标（在此为生成可执行文件），如需编译生成库则使用 add_library(<name> [STATIC | SHARED | MODULE] <srcs>)
add_executable(${PROJECT_NAME} 
	${SROUCES_LIST}
)


# 指定需要链接的库文件搜索路径
#link_directories()

# 指定需要链接的库
#target_link_libraries(${PROJECT_NAME} )


# 指定编译目标的下层依赖项（类似于Makefile的伪目标）
# 当编译上层target时，自动检查下层依赖项是否已经生成。如果没有则先编译下层依赖项，然后再编译上层target
#add_dependencies(<target> [<target-dependency>]...)


# 添加一个子目录并构建该子目录
#add_subdirectory (<source_dir> [binary_dir] [EXCLUDE_FROM_ALL])


###################################### 链接第三方库 #############################################

###################################### Boost Start ############################################
# 查找 Boost_1_71_0 的特定的库
find_package(Boost 1.71.0 REQUIRED COMPONENTS thread chrono)
# 包含Boost头文件目录
include_directories(${PROJECT_NAME} ${Boost_INCLUDE_DIRS})
message("Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message("Boost_LIBRARIES: ${Boost_LIBRARIES}")
# 链接Boost库文件
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
###################################### Boost End ##############################################

####################################### OpenCV Start ##########################################
# 设置OpenCVModule的路径
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "/home/undefined/Downloads/opencv-3.4.1/build")
# 查找 OpenCV (不要求版本)的头文件和库文件路径以及库文件
#find_package(OpenCV REQUIRED)
# 包含OpenCV头文件目录
#include_directories(${PROJECT_NAME} ${OpenCV_INCLUDE_DIRS})
# 链接OpenCV库文件
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
###################################### OpenCV End #############################################

###################################### OpenGL Start ###########################################
# opengl 头文件路径保存在 OPENGL_INC_DIR 变量
#set(OPENGL_INC_DIR /usr/local/include /usr/include /usr/include/GL)
# opengl 链接库路径保存在 OPENGL_LDFLAGS 变量
#set(OPENGL_LDFLAGS /usr/local/lib /usr/lib64)
# opengl 链接库名称保存在 OPENGL_LIBS 变量
#set(OPENGL_LIBS GL GLU glut GLEW gltools glfw3 X11 Xxf86vm Xrandr pthread Xi dl Xinerama Xcursor m)
# OpenGL头文件目录
#include_directories(${PROJECT_NAME} ${OPENGL_INC_DIR})
# OpenGL库文件目录
#link_directories(${OPENGL_LDFLAGS})
# 链接OpenGL库文件
#target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBS})
###################################### OpenGL End ############################################
