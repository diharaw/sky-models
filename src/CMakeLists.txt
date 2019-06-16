cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

find_program(CLANG_FORMAT_EXE NAMES "clang-format" DOC "Path to clang-format executable")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(SKY_MODELS_SOURCES ${PROJECT_SOURCE_DIR}/src/main.cpp
                       ${PROJECT_SOURCE_DIR}/src/bruneton_sky_model.h
                       ${PROJECT_SOURCE_DIR}/src/bruneton_sky_model.cpp
                       ${PROJECT_SOURCE_DIR}/src/preetham_sky_model.h
                       ${PROJECT_SOURCE_DIR}/src/preetham_sky_model.cpp
                       ${PROJECT_SOURCE_DIR}/src/hosek_wilkie_sky_model.h
                       ${PROJECT_SOURCE_DIR}/src/hosek_wilkie_sky_model.cpp
                       ${PROJECT_SOURCE_DIR}/src/sky_model.h
                       ${PROJECT_SOURCE_DIR}/src/hosek_data_rgb.inl)

if (EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

if(APPLE)
    add_executable(SkyModels MACOSX_BUNDLE ${SKY_MODELS_SOURCES})
    set(MACOSX_BUNDLE_BUNDLE_NAME "com.dihara.skymodels") 
else()
    add_executable(SkyModels ${SKY_MODELS_SOURCES}) 
endif()

target_link_libraries(SkyModels dwSampleFramework)

if (EMSCRIPTEN)
    set_target_properties(SkyModels PROPERTIES LINK_FLAGS "--embed-file ${PROJECT_SOURCE_DIR}/shader/fs.glsl@shader/fs.glsl --embed-file ${PROJECT_SOURCE_DIR}/shader/fs.glsl@shader/fs.glsl -O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_GLFW=3 -s USE_WEBGL2=1")
endif()

if (APPLE)
    add_custom_command(TARGET SkyModels POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shader $<TARGET_FILE_DIR:SkyModels>/SkyModels.app/Contents/Resources/shader)
    add_custom_command(TARGET SkyModels POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data/mesh $<TARGET_FILE_DIR:SkyModels>/SkyModels.app/Contents/Resources/mesh)
else()
    add_custom_command(TARGET SkyModels POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shader $<TARGET_FILE_DIR:SkyModels>/shader)
    add_custom_command(TARGET SkyModels POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data/mesh $<TARGET_FILE_DIR:SkyModels>/mesh)
endif()

if(CLANG_FORMAT_EXE)
    add_custom_target(clang-format-project-files COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${SKY_MODELS_SOURCES})
endif()

set_property(TARGET SkyModels PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$(Configuration)")