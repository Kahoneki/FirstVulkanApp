cmake_minimum_required(VERSION 3.28)
project(FirstVulkanApp)
set(CMAKE_CXX_STANDARD 23)

set(PROJECT_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/out")

#Find all .cpp and .h files recursively (including those in /out/)
file(GLOB_RECURSE ALL_PROJECT_FILES
    "${CMAKE_SOURCE_DIR}/*.h"
    "${CMAKE_SOURCE_DIR}/*.cpp"
)

#Create an empty list to store the clean source files (those not in the /out/ directory)
set(CLEAN_SOURCE_FILES "")

#Loop through every file path that the glob found and check if in the project output dir (/out/)
foreach(FILE_PATH ${ALL_PROJECT_FILES})
    string(FIND "${FILE_PATH}" "${PROJECT_OUTPUT_DIR}" IS_IN_PROJECT_OUTPUT_DIR) #returns 0 if substring is found, otherwise returns -1
    if(IS_IN_PROJECT_OUTPUT_DIR EQUAL -1)
        #File not in output dir, add to CLEAN_SOURCE_FILES
        list(APPEND CLEAN_SOURCE_FILES ${FILE_PATH})
    endif()
endforeach()

#Output all the clean source files to be able to check that cleansing worked as expected
message(STATUS "--- Final Filtered Source Files ---")
foreach(FILE ${CLEAN_SOURCE_FILES})
    message(STATUS "  ${FILE}")
endforeach()
message(STATUS "------------------------------------")

#Use the clean list to create the executable
add_executable(FirstVulkanApp ${CLEAN_SOURCE_FILES})

#Link Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(FirstVulkanApp PRIVATE Vulkan::Vulkan)