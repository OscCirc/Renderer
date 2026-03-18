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
# STB_IMAGE_IMPLEMENTATION 已在 image.cpp 中定义，此处不重复定义
# 屏蔽 MSVC 不安全函数警告 (sscanf 等)
target_compile_definitions(SoftRenderer PRIVATE _CRT_SECURE_NO_WARNINGS)

# 查找 stb 头文件，兼容 vcpkg toolchain 未指定的情况
find_path(STB_INCLUDE_DIRS "stb_image.h"
    HINTS
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
        "D:/repo/vcpkg/installed/x64-windows/include"
        "C:/vcpkg/installed/x64-windows/include"
)
if(NOT STB_INCLUDE_DIRS)
    message(FATAL_ERROR
        "stb_image.h not found.\n"
        "请确认已执行: vcpkg install stb\n"
        "并在 cmake 配置时指定 toolchain:\n"
        "  -DCMAKE_TOOLCHAIN_FILE=D:/repo/vcpkg/scripts/buildsystems/vcpkg.cmake"
    )
endif()
target_include_directories(SoftRenderer PRIVATE ${STB_INCLUDE_DIRS})

# --- 按目录树生成 VS Filter，镜像磁盘目录结构 ---
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES} ${HEADER_FILES})

# --- Post-Build Steps ---
add_custom_command(TARGET SoftRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:SoftRenderer>/assets
    COMMENT "Copying assets directory"
)