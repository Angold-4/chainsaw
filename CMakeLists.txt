cmake_minimum_required(VERSION 3.10) 

project(Chainsaw)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define the output directory
set(OUTPUT_DIR "$ENV{HOME}/.chainsaw")

# Create executables for cf, parsesubmit, and parseuser
add_executable(cf src/cf.cpp)
target_link_libraries(cf PRIVATE curl pthread)

add_executable(parsesubmit src/parsesubmit.cpp)
target_link_libraries(parsesubmit PRIVATE curl pthread)

add_executable(parseuser src/parseuser.cpp)

# Check platform for debug compilation
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") # MacOS
    add_executable(csdebug src/debug/main.cpp src/debug/editor.cpp src/debug/inter.cpp)
    target_compile_definitions(csdebug PRIVATE MAC_OS)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_executable(csdebug src/debug/main.cpp src/debug/editor.cpp src/debug/inter.cpp)
    target_compile_definitions(csdebug PRIVATE LINUX)
endif()

# Copy the template.cpp to the output directory
add_custom_target(copy_template ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/src/template.cpp ${OUTPUT_DIR}/template.cpp
    DEPENDS ${CMAKE_SOURCE_DIR}/src/template.cpp
)

# Set the output directory for the binaries
set_target_properties(cf parsesubmit parseuser csdebug
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
)

# Install the chainsaw script
install(PROGRAMS src/chainsaw.sh 
        DESTINATION /usr/local/bin/ 
        RENAME chainsaw)
