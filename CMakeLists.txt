cmake_minimum_required(VERSION 3.2)

# zf_log target (required)
set(HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(HEADERS door.h)
set(SOURCES door.cpp ansicolor.cpp lines.cpp panel.cpp anyoption.cpp)

# add_subdirectory(opendoors)

###########
# Debug or Release
###########
if (NOT CMAKE_BUILD_TYPE)
  ## set default to Debug
  set(CMAKE_BUILD_TYPE Debug)  # override with -DCMAKE_BUILD_TYPE=Release
  message("==> CMAKE_BUILD_TYPE empty. Changing it to Debug.")
else()
  message("==> CMAKE_BUILD_TYPE == ${CMAKE_BUILD_TYPE}.")
endif()

## https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_macros.html
## During Debug, use debug version of libstdc++ (asserts on access to invalid iterators, etc!)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

##############
# C++ Standard
##############
set(CMAKE_CXX_STANDARD   14)
# set(CMAKE_CXX_STANDARD   17)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic-errors")
endif()

add_library(door++ ${HEADERS} ${SOURCES})

# target_link_libraries(door++ pthread)

target_include_directories(door++ PUBLIC $<BUILD_INTERFACE:${HEADERS_DIR}>)

if(ZF_LOG_LIBRARY_PREFIX)
	target_compile_definitions(door++ PRIVATE "ZF_LOG_LIBRARY_PREFIX=${ZF_LOG_LIBRARY_PREFIX}")
endif()

