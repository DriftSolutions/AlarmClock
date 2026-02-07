cmake_minimum_required (VERSION 3.10)

SET(PROJECT_BINARY_DIR ${CMAKE_BINARY_DIR})
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

project (AlarmClock)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(${CMAKE_ROOT}/Modules/FindPkgConfig.cmake)

SET(LIBS pthread dl)

pkg_search_module(LIBCURL REQUIRED libcurl)
include_directories(${LIBCURL_INCLUDE_DIRS})
link_directories(${LIBCURL_LIBRARY_DIRS})
SET(LIBS ${LIBS} ${LIBCURL_LIBRARIES})

pkg_search_module(UNIVALUE REQUIRED libunivalue univalue)
include_directories(${UNIVALUE_INCLUDE_DIRS})
link_directories(${UNIVALUE_LIBRARY_DIRS})
SET(LIBS ${LIBS} ${UNIVALUE_LIBRARIES})

pkg_check_modules(SDL REQUIRED sdl2 SDL2_image SDL2_ttf SDL2_gfx SDL2_mixer)
include_directories(${SDL_INCLUDE_DIRS})
link_directories(${SDL_LIBRARY_DIRS})
SET(LIBS ${LIBS} ${SDL_LIBRARIES})

link_directories(/usr/local/lib)

add_definitions(-fPIC -pthread -DDSL_STATIC -DENABLE_CURL)
add_definitions(-D_REENTRANT -D_THREAD_SAFE -D_POSIX_C_SOURCE=200112 -D_FORTIFY_SOURCE=2 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE)

IF(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)
message(STATUS "Enabling 64-bit support...")
link_directories(/usr/lib/x86_64-linux-gnu)
ELSE(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)
# 32-bit only includes
link_directories(/usr/lib/i386-linux-gnu)
ENDIF(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64 OR CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)

function (OptTest option)
	check_cxx_compiler_flag(${option} OPTCHK)
	IF(OPTCHK)
		message(STATUS "GCC has ${option}")
		add_definitions(${option})
	ELSE(OPTCHK)
		message(STATUS "GCC does not have ${option}")
	ENDIF(OPTCHK)
	UNSET(OPTCHK CACHE)
endfunction(OptTest)
function (OptTest2 option def)
	check_cxx_compiler_flag(${option} OPTCHK)
	IF(OPTCHK)
		message(STATUS "GCC has ${option}")
		add_definitions(${option} ${def})
	ELSE(OPTCHK)
		message(STATUS "GCC does not have ${option}")
	ENDIF(OPTCHK)
	UNSET(OPTCHK CACHE)
endfunction(OptTest2)
OptTest2(-fvisibility=hidden -DHAVE_VIS)
OptTest2(-mrdrnd -DGCC_RDRAND)
OptTest(-fstack-protector-all)
#OptTest(-Wstack-protector)
OptTest(-Wno-pragmas)
OptTest(-Wno-write-strings)
OptTest(-Wno-format-contains-nul)
OptTest(-Werror=format-security)
OptTest(-Werror=format-extra-args)

#optimizations
add_definitions(-O3)
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -s")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -s")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -s")
add_definitions(-DNDEBUG)

#or enable debugging
#add_definitions(-D_GLIBCXX_DEBUG -DDEBUG -D_DEBUG)
#set(DEBUG 1)
#add_definitions(-g -ggdb)

SET(DSL_DIRECTORY ${PROJECT_SOURCE_DIR}/DSL/dslsrc)
include_directories(${DSL_DIRECTORY})

add_subdirectory (DSL)
SET(LIBS acdsl ${LIBS})

AUX_SOURCE_DIRECTORY(. SRCFILES)
add_executable (alarm_clock ${SRCFILES})
TARGET_LINK_LIBRARIES(alarm_clock ${LIBS})
