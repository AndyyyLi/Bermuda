cmake_minimum_required(VERSION 3.1)
project(bermuda)

# Set c++11
# https://stackoverflow.com/questions/10851247/how-to-activate-c-11-in-cmake
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif()
set(CMAKE_CXX_STANDARD 14)

# nice hierarchichal structure in MSVC
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# nice for lsp
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find OS
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(IS_OS_MAC 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(IS_OS_LINUX 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(IS_OS_WINDOWS 1)
else()
  message(FATAL_ERROR "OS ${CMAKE_SYSTEM_NAME} was not recognized")
endif()

# Create executable target

# Generate the shader folder location to the header
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ext/project_path.hpp.in"
               "${CMAKE_CURRENT_SOURCE_DIR}/ext/project_path.hpp")

# You can switch to use the file GLOB for simplicity but at your own risk
file(
  GLOB
  SOURCE_FILES
  src/ecs/*.cpp
  src/ecs/*.hpp
  src/config/*.cpp
  src/config/*.hpp
  src/systems/*.cpp
  src/systems/*.hpp
  src/systems/abilities/*.cpp
  src/systems/abilities/*.hpp
  src/systems/ai/*.cpp
  src/systems/ai/*.hpp
  src/systems/entities/*.hpp
  src/systems/entities/*.cpp
  src/systems/consumables/*.hpp
  src/systems/consumables/*.cpp
  src/systems/enemies/*.cpp
  src/systems/enemies/*.hpp
  src/systems/levels/*.cpp
  src/systems/levels/*.hpp
  src/systems/map/*.cpp
  src/systems/map/*.hpp
  src/systems/oxygen/*.cpp
  src/systems/oxygen/*.hpp
  src/systems/physics/*.cpp
  src/systems/physics/*.hpp
  src/systems/collisions/*.cpp
  src/systems/collisions/*.hpp
  src/systems/audio/*.cpp
  src/systems/audio/*.hpp
  src/systems/player/*.cpp
  src/systems/player/*.hpp
  src/systems/oxygen/*.cpp
  src/systems/oxygen/*.hpp
  src/systems/rendering/*.cpp
  src/systems/rendering/*.hpp
  src/systems/saving/*.cpp
  src/systems/saving/*.hpp
  src/systems/texts/*.cpp
  src/systems/texts/*.hpp  
  src/systems/world/*.cpp
  src/systems/world/*.hpp
  src/systems/world_state/*.cpp
  src/systems/world_state/*.hpp
  src/util/*.cpp
  src/util/*.hpp
  src/components/*.cpp
  src/components/*.hpp
  src/*.cpp
  src/*.hpp)

# external libraries will be installed into /usr/local/include and
# /usr/local/lib but that folder is not automatically included in the search on
# MACs
if(IS_OS_MAC)
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
  # 2024-09-24 - added for M-series Mac's
  include_directories(/opt/homebrew/include)
  link_directories(/opt/homebrew/lib)
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC src/)
target_include_directories(${PROJECT_NAME} PRIVATE src/ecs/)
target_include_directories(${PROJECT_NAME} PRIVATE src/config/)
target_include_directories(${PROJECT_NAME} PRIVATE src/components/)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/abilities)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/ai)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/enemies)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/levels)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/map)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/oxygen)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/physics)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/collisions)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/player)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/audio)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/rendering)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/saving)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/texts)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/world)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/world_state)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/entities)
target_include_directories(${PROJECT_NAME} PRIVATE src/systems/consumables)
target_include_directories(${PROJECT_NAME} PRIVATE src/util)

# Added this so policy CMP0065 doesn't scream
set_target_properties(${PROJECT_NAME} PROPERTIES ENABLE_EXPORTS 0)

# External header-only libraries in the ext/
target_include_directories(${PROJECT_NAME} PUBLIC ext/stb_image/)
target_include_directories(${PROJECT_NAME} PUBLIC ext/gl3w)
target_include_directories(${PROJECT_NAME} PUBLIC ext/json)

# Find OpenGL
find_package(OpenGL REQUIRED)

if(OPENGL_FOUND)
  target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENGL_gl_LIBRARY})
endif()

set(glm_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/glm/cmake/glm) # if necessary
find_package(glm REQUIRED)

# GLFW, SDL2 could be precompiled (on windows) or installed by a package manager
# (on OSX and Linux)
if(IS_OS_LINUX OR IS_OS_MAC)
  # Try to find packages rather than to use the precompiled ones Since we're on
  # OSX or Linux, we can just use pkgconfig.
  find_package(PkgConfig REQUIRED)

  pkg_search_module(GLFW REQUIRED glfw3)

  pkg_search_module(SDL2 REQUIRED sdl2)
  pkg_search_module(SDL2MIXER REQUIRED SDL2_mixer)

  # Link Frameworks on OSX
  if(IS_OS_MAC)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(CF_LIBRARY CoreFoundation)
    target_link_libraries(${PROJECT_NAME} PUBLIC ${COCOA_LIBRARY} ${CF_LIBRARY})
  endif()

  # Increase warning level
  target_compile_options(${PROJECT_NAME} PUBLIC "-Wall")
elseif(IS_OS_WINDOWS)
  # https://stackoverflow.com/questions/17126860/cmake-link-precompiled-library-depending-on-os-and-architecture
  set(GLFW_FOUND TRUE)
  set(SDL2_FOUND TRUE)

  set(GLFW_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/include")
  set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/include/SDL")

  if(${CMAKE_SIZEOF_VOID_P} MATCHES "8")
    set(GLFW_LIBRARIES
        "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/lib/glfw3dll-x64.lib")
    set(SDL2_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/SDL2-x64.lib")
    set(SDL2MIXER_LIBRARIES
        "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/SDL2_mixer-x64.lib")

    set(GLFW_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/lib/glfw3-x64.dll")
    set(SDL2_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/SDL2-x64.dll")
    set(SDL2MIXER_DLL
        "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/SDL2_mixer-x64.dll")
  else()
    set(GLFW_LIBRARIES
        "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/lib/glfw3dll-x86.lib")
    set(SDL2_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/SDL2-x86.lib")
    set(SDL2MIXER_LIBRARIES
        "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/SDL2_mixer-x86.lib")

    set(GLFW_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/glfw/lib/glfw3-x86.dll")
    set(SDL2_DLL "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/SDL2-x86.dll")
    set(SDL2MIXER_DLL
        "${CMAKE_CURRENT_SOURCE_DIR}/ext/sdl/lib/SDL2_mixer-x86.dll")
  endif()

  # FreeType - Windows-specific (x64 only)
  set (FREETYPE_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/ext/freetype/include")
  set (FREETYPE_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/ext/freetype/release static/vs2015-2022/win64/freetype.lib")

  # Copy and rename dlls
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GLFW_DLL}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/glfw3.dll")
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2_DLL}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL2.dll")
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2MIXER_DLL}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL2_mixer.dll")

  target_compile_options(
    ${PROJECT_NAME}
    PUBLIC
      # increase warning level
      "/W4"
      # Turn warning "not all control paths return a value" into an error
      "/we4715"
      # use sane exception handling, rather than trying to catch segfaults and
      # allowing resource leaks and UB. Yup... See "Default exception handling
      # behavior" at
      # https://docs.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model?view=vs-2019
      "/EHsc"
      # turn warning C4239 (non-standard extension that allows temporaries to be
      # bound to non-const references, yay microsoft) into an error
      "/we4239")
endif()

# Can't find the include and lib. Quit.
if (NOT GLFW_FOUND OR NOT SDL2_FOUND)
   if (NOT GLFW_FOUND)
        message(FATAL_ERROR "Can't find GLFW." )
   else ()
        message(FATAL_ERROR "Can't find SDL2." )
   endif()
endif()

find_package(Freetype REQUIRED)

if(TARGET Freetype AND NOT TARGET Freetype::Freetype)
     add_library(Freetype::Freetype ALIAS freetype) # target freetype is defined by freetype-targets.cmake
     # might need to add freetype to global scope if cmake errors here
     # alternativly if the above does not work for you you can use
     # add_library(Freetype::Freetype INTERFACE IMPORTED)
     # target_link_libraries(Freetype::Freetype INTERFACE freetype)
endif()
 
if(NOT TARGET Freetype::Freetype)
     # insert error here
     # or create the target correctly (see cmakes newer FindFreetype.cmake)
     message(FATAL_ERROR "Can't find FreeType (fonts)." )
endif()
 
 include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/freetype/include")

target_include_directories(${PROJECT_NAME} PUBLIC ${GLFW_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PUBLIC ${GLFW_LIBRARIES} ${SDL2_LIBRARIES} ${SDL2MIXER_LIBRARIES} glm::glm ${FREETYPE_LIBRARY})

# Needed to add this
if(IS_OS_LINUX)
    target_link_libraries(${PROJECT_NAME} PUBLIC glfw ${CMAKE_DL_LIBS})
endif()

# This might cause problems but I'm a sweat, add git hash to the save files
# We ball
# - Ghadhi
execute_process(
    COMMAND git rev-parse HEAD
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

target_compile_definitions(${PROJECT_NAME} PRIVATE GIT_HASH="${GIT_HASH}")
message(STATUS "Git hash: ${GIT_HASH}")
