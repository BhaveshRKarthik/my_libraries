# CMakeList.txt : CMake project for my_libraries, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (my_libraries "my_libraries.cpp" "my_libraries.h" "include/bhavesh_setup.h" "include/bhavesh_silence.h" "include/bhavesh_matrix.h" )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET my_libraries PROPERTY CXX_STANDARD 14)
endif()

# TODO: Add tests and install targets if needed.
