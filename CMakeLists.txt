cmake_minimum_required(VERSION 3.22.1)
# 定义项目名称和使用的 C++ 标准
project(ReactorIM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 告诉编译器去哪里找头文件
include_directories(${PROJECT_SOURCE_DIR}/include)

# 收集 src 目录及其子目录下所有的 .cpp 文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 链接多线程库 (Reactor 模型需要用到 std::thread 或 pthread)
find_package(Threads REQUIRED)

# 生成可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 将多线程库链接到我们的目标程序上
target_link_libraries(ReactorIM PRIVATE mysqlclient)