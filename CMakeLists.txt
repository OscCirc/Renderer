cmake_minimum_required(VERSION 3.15)
project(SoftRenderer)

# --- Project Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 警告级别设置
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 配置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Find Dependencies ---
find_package(Eigen3 CONFIG REQUIRED)


# --- Source Files ---
# 自动收集 src 目录下的所有 .cpp 文件
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

# 自动收集 include 目录下的所有头文件
file(GLOB_RECURSE HEADER_FILES "include/*.h" "include/*.hpp")

# 主可执行文件 - 添加头文件到目标
add_executable(SoftRenderer ${SOURCE_FILES} ${HEADER_FILES})

# --- Include Directories ---
target_include_directories(SoftRenderer PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    )

# --- Link Libraries ---
target_link_libraries(SoftRenderer PRIVATE
    Eigen3::Eigen
    )

# --- Third-party Header-Only Libraries (stb_image) ---
target_compile_definitions(SoftRenderer PRIVATE STB_IMAGE_IMPLEMENTATION)

# --- 创建头文件分组 ---
source_group("Header Files" FILES ${HEADER_FILES})

# --- Post-Build Steps ---
add_custom_command(TARGET SoftRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:SoftRenderer>/assets
    COMMENT "Copying assets directory"
)