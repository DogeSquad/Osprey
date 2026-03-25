cmake_minimum_required (VERSION 3.29)

project (Osprey)
set(${SHADER_SLANG_TARGET} Shaders)


find_package (glfw3 REQUIRED)
find_package (glm REQUIRED)
find_package (Vulkan REQUIRED)
find_package (tinyobjloader REQUIRED)
# find_package (tinygltf REQUIRED)
find_package (KTX REQUIRED)
find_package(stb REQUIRED)
find_package(imgui REQUIRED)
find_package(imguizmo  CONFIG REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(nfd REQUIRED)
set(STB_INCLUDEDIR ${stb_INCLUDE_DIRS})

# Add Shader compiler
add_executable (glslang::validator IMPORTED)
find_program (GLSLANG_VALIDATOR "glslangValidator" HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
set_property (TARGET glslang::validator PROPERTY IMPORTED_LOCATION "${GLSLANG_VALIDATOR}")
find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)





# Add dummy Vulkan library
add_library(VulkanCppModule INTERFACE)
add_library(Vulkan::cppm ALIAS VulkanCppModule)
target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
target_compile_definitions(VulkanCppModule
        INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=0 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)


file(GLOB_RECURSE SRC_FILES "src/*.h" "src/*.cpp")
file(GLOB_RECURSE SHADER_FILES "shaders/*.slang")

set_source_files_properties(${SHADER_FILES} PROPERTIES LANGUAGE SLANG)
source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    PREFIX "Shaders"
    FILES ${SHADER_FILES}
)


add_executable(${PROJECT_NAME} ${SRC_FILES} ${SHADER_FILES})


set_target_properties (${PROJECT_NAME} PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
set_target_properties (${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
target_link_libraries (${PROJECT_NAME} Vulkan::cppm glfw glm::glm imgui::imgui imguizmo::imguizmo yaml-cpp::yaml-cpp nfd::nfd)
target_include_directories (${PROJECT_NAME} PRIVATE ${STB_INCLUDEDIR})
target_include_directories (${PROJECT_NAME} PRIVATE src)

if(WIN32)
    if(${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
        set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
    endif()
endif()


function (add_slang_shader_target TARGET)
  cmake_parse_arguments ("SHADER" "" "CHAPTER_NAME" "SOURCES" ${ARGN})
  set (SHADERS_DIR ${SHADER_CHAPTER_NAME}/shaders)
  set (SHADERS_SOURCE_DIR ${SHADER_CHAPTER_NAME}/../shaders)

  # discover all .slang files automatically
  file (GLOB SLANG_SOURCES CONFIGURE_DEPENDS "${SHADERS_SOURCE_DIR}/*.slang")

  add_custom_command (
    OUTPUT ${SHADERS_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
  )

  set (SPV_OUTPUTS)

  foreach (SOURCE ${SLANG_SOURCES})
    get_filename_component (SHADER_NAME ${SOURCE} NAME_WE)
    set (SPV_OUTPUT ${SHADERS_DIR}/${SHADER_NAME}.spv)

    add_custom_command (
      OUTPUT  ${SPV_OUTPUT}
      COMMAND ${SLANGC_EXECUTABLE} ${SOURCE} -target spirv -profile spirv_1_4+spvRayQueryKHR -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o ${SHADER_NAME}.spv
      WORKING_DIRECTORY ${SHADERS_DIR}
      DEPENDS ${SHADERS_DIR} ${SOURCE}
      COMMENT "Compiling ${SHADER_NAME}.slang -> ${SHADER_NAME}.spv"
      VERBATIM
    )

    list (APPEND SPV_OUTPUTS ${SPV_OUTPUT})
  endforeach()

  add_custom_target (${TARGET} DEPENDS ${SPV_OUTPUTS})
endfunction()


set(CHAPTER_SHADER main_shader)
set(SHADER_TARGET Shader)
file(GLOB SHADER_SLANG_SOURCES shaders/${CHAPTER_SHADER}.slang)
if(SHADER_SLANG_SOURCES)
    add_slang_shader_target( ${SHADER_TARGET} CHAPTER_NAME ${PROJECT_NAME} SOURCES ${SHADER_SLANG_SOURCES})
    add_dependencies(${PROJECT_NAME} ${SHADER_TARGET})
endif()


