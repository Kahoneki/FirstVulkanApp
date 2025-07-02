cmake_minimum_required(VERSION 3.31)
project(FirstVulkanApp)

set(CMAKE_CXX_STANDARD 23)

add_executable(FirstVulkanApp VKApp.cpp
        VKDebugAllocator.cpp
        VKDebugAllocator.h
        VKApp.h
        main.cpp)

find_package(Vulkan REQUIRED)
target_link_libraries(FirstVulkanApp PRIVATE Vulkan::Vulkan)