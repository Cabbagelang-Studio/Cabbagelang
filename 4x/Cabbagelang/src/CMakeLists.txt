# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(Cabbagelang)

# Save compile date
string(TIMESTAMP COMPILE_DATE "%Y-%m-%d")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/configure/CompileDate.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/configure/CompileDate.h
)


# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# LLVM configuration
find_package(LLVM REQUIRED CONFIG)

# Set release flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto")
    set(CMAKE_EXE_LINKER_FLAGS "-static -flto")
    set(CMAKE_BUILD_TYPE Release)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /MT /GL /W0")
    set(CMAKE_EXE_LINKER_FLAGS "/LTCG /INCREMENTAL:NO /DEBUG:NONE")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM include dir: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")
message(STATUS "Release Compiler flags: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "Release Linker flags: ${CMAKE_EXE_LINKER_FLAGS}")

# Include directories for the project
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/include)
include_directories(${CMAKE_SOURCE_DIR}/configure)

# Link the LLVM libraries
include_directories(${LLVM_INCLUDE_DIRS} SYSTEM)
llvm_map_components_to_libnames(llvm_libs core support irreader passes)

# Set the source files
set(SOURCES
    Main.cc
    log/log.cc
    log/colorize.cc
    utils/exceptions.cc
    utils/str_handle.cc
    utils/debug.cc
    utils/init.cc
    utils/options.cc
    symbols/ClassNames.cc
    symbols/Value.cc
    lexer/Token.cc
    Lexer/Lexer.cc
    parser/Ast.cc
    parser/Parser.cc
    codegen/CodeGen.cc
    codegen/ClassMem.cc
    codegen/Function.cc
    args/Args.cc
    compiler/Compiler.cc

    Cabbagelang/Class.cc
    Cabbagelang/cast.cc
    Cabbagelang/Exception.cc
    Cabbagelang/String.cc
    Cabbagelang/Generic.cc
    Cabbagelang/Types.cc
)

# Create the executable file
add_executable(cabbage ${SOURCES})

# Link the LLVM libraries to the executable
target_link_libraries(cabbage ${llvm_libs})
add_definitions(${LLVM_DEFINITIONS})

# Custom command to copy the executable to the parent directory after building
add_custom_command(TARGET cabbage POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:cabbage>
    ${CMAKE_BINARY_DIR}/../${OUTPUT_NAME}
)