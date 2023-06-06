cmake_minimum_required(VERSION 3.2.2)
# -o 起名
project(demo)
#设置c++ -std
set(CMAKE_CXX_STANDARD 11)

# -g
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 头文件搜索路径 -I
include_directories(/usr/include)

# 库文件搜索路径 -L
link_directories(/usr/local/lib)

# 输出bin可执行文件位置        #PROJECT_SOURCE_DIR cmakelists.txt所在工程根目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 用到的cpp
#aux_source_directory(. SRC_LIST) # 若.cpp等源文件过多，指定路径下所有cpp都加入变量名SRC_LIST中
set(SRC_LIST ./muduo_server.cpp ../../src/client/main.cpp)
add_executable(${PROJECT_NAME} ${SRC_LIST}) #必填，告知哪个文件作为编译文件后

## 链接 库 -l lib和尾部.a .so等不写
target_link_libraries(${PROJECT_NAME} muduo_net muduo_base pthread)

# 添加根目录下全局编译设定 ，可以沿着该设定去执行这些文件夹下的cmake
# add_subdirectory(文件夹下其他文件夹路径)
