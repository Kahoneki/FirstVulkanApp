cmake_minimum_required(VERSION 3.28)
project(FirstVulkanApp)
set(CMAKE_CXX_STANDARD 23)
set(PROJECT_OUTPUT_DIR "${CMAKE_BINARY_DIR}")


#Add GLFW
include(FetchContent)
set(GLFW_VERSION "3.4")
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG ${GLFW_VERSION}
)
#Prevent GLFW from trying to find packages (e.g.: Doxygen)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)


#Add stb_image
FetchContent_Declare(
        stb_image
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master #stb on master is stable
)
FetchContent_MakeAvailable(stb_image)



#Add GLM
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.9.8 #Known stable version
)
FetchContent_MakeAvailable(glm)


#Find all .cpp and .h files recursively (including those in /out/)
file(GLOB_RECURSE ALL_PROJECT_FILES
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)



#Output all the clean source files to be able to check that cleansing worked as expected
message(STATUS "--- Final Filtered Source Files --- dir: ${PROJECT_OUTPUT_DIR}")
foreach(FILE ${ALL_PROJECT_FILES})
    message(STATUS "  ${FILE}")
endforeach()
message(STATUS "------------------------------------")

#Use the clean list to create the executable
add_executable(FirstVulkanApp ${ALL_PROJECT_FILES})


#Link Vulkan, GLFW, and stb_image
target_include_directories(FirstVulkanApp PRIVATE ${stb_image_SOURCE_DIR} ${glm_SOURCE_DIR})
find_package(Vulkan REQUIRED)
target_link_libraries(FirstVulkanApp PRIVATE Vulkan::Vulkan glfw)







#Shader compilation
find_program(
    GLSLC_EXECUTABLE
    NAMES glslc
    HINTS "$ENV{VULKAN_SDK}/bin"
    REQUIRED
    DOC "Path to the GLSL to SPIR-V compiler"
)
message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")

#Define shader files
file(GLOB_RECURSE SHADER_SOURCES
    "${CMAKE_SOURCE_DIR}/Shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/Shaders/*.frag"
)
message(STATUS "--- Found Shader Files ---")
foreach(FILE ${SHADER_SOURCES})
    message(STATUS "  ${FILE}")
endforeach()
message(STATUS "------------------------------------")

#Get list of all the full paths of all the compiled .spv files
set(SPIRV_OUTPUTS "")
foreach(SHADER_SRC ${SHADER_SOURCES})
    get_filename_component(SHADER_FILENAME ${SHADER_SRC} NAME)
    set(SHADER_SPV "${PROJECT_BINARY_DIR}/${SHADER_FILENAME}.spv")
    list(APPEND SPIRV_OUTPUTS ${SHADER_SPV})

    #Add custom build command for this shader
    add_custom_command(
        OUTPUT ${SHADER_SPV}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SRC} -o ${SHADER_SPV}
        DEPENDS ${SHADER_SRC}
        COMMENT "Compiling ${SHADER_SRC} to ${SHADER_SPV}"
    )
endforeach()

#Create a phony target that depends on all the compiled shaders then add it to the executable
add_custom_target(
    Shaders
    DEPENDS ${SPIRV_OUTPUTS}
)
add_dependencies(FirstVulkanApp Shaders)








#Copy all resource files to the output directory, maintaining directory structure
file(COPY "${CMAKE_SOURCE_DIR}/Resource Files/" DESTINATION "${CMAKE_BINARY_DIR}/Resource Files/")