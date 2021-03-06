#
# file: CMakeLists.txt
#
# This is the main CMakeLists.txt for distortos
#
# author: Copyright (C) 2018 Kamil Szczygiel http://www.distortec.com http://www.freddiechopin.info
#
# This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

cmake_minimum_required(VERSION 3.1)
project(distortos)

include(distortos-utilities)

#-----------------------------------------------------------------------------------------------------------------------
# local functions
#-----------------------------------------------------------------------------------------------------------------------

#
# Adds given folders to doxygen `INPUT`, `INCLUDE_PATH` and/or `EXCLUDE`.
#
# `doxygen([INPUT inputFolder ...] [INCLUDE_PATH includePathFolder ...] [EXCLUDE excludeFolder ...])`
#

function(doxygen)
	cmake_parse_arguments(PARSE_ARGV 0 DOXYGEN "" "" "EXCLUDE;INCLUDE_PATH;INPUT")
	if(DOXYGEN_EXCLUDE)
		set(envDoxygenExclude "$ENV{doxygenExclude}")
		list(APPEND envDoxygenExclude ${DOXYGEN_EXCLUDE})
		set(ENV{doxygenExclude} "${envDoxygenExclude}")
	endif()
	if(DOXYGEN_INCLUDE_PATH)
		set(envDoxygenIncludePath "$ENV{doxygenIncludePath}")
		list(APPEND envDoxygenIncludePath ${DOXYGEN_INCLUDE_PATH})
		set(ENV{doxygenIncludePath} "${envDoxygenIncludePath}")
	endif()
	if(DOXYGEN_INPUT)
		set(envDoxygenInput "$ENV{doxygenInput}")
		list(APPEND envDoxygenInput ${DOXYGEN_INPUT})
		set(ENV{doxygenInput} "${envDoxygenInput}")
	endif()
endfunction()

#
# Recursively gets `property` from the `target` and all of its (interface) link libraries, saving the result to
# `outputVariable`.
#

function(getTargetPropertyRecursive outputVariable target property)
	get_target_property(type ${target} TYPE)
	if(type STREQUAL INTERFACE_LIBRARY)
		set(interfacePrefix INTERFACE_)
	else()
		unset(interfacePrefix)
	endif()

	get_target_property(linkLibraries ${target} "${interfacePrefix}LINK_LIBRARIES")
	foreach(linkLibrary ${linkLibraries})
		if(linkLibrary)
			getTargetPropertyRecursive(${outputVariable} ${linkLibrary} ${property})
		endif()
	endforeach()

	get_target_property(propertyValue ${target} "${interfacePrefix}${property}")
	if(propertyValue)
		list(APPEND ${outputVariable} ${propertyValue})
		list(REMOVE_DUPLICATES ${outputVariable})
	endif()

	set(${outputVariable} ${${outputVariable}} PARENT_SCOPE)
endfunction()

#
# Loads configuration variables from kconfig file named `filename`.
#

function(loadConfiguration filename)
	file(STRINGS ${filename} lines)
	foreach(line ${lines})
		if("${line}" MATCHES "^([^=]+)=(.*)$")
			set(key ${CMAKE_MATCH_1})
			set(value ${CMAKE_MATCH_2})
			if(${value} STREQUAL y)	# bool? convert to ON
				set(value ON)
			elseif(${value} MATCHES "^\"(.*)\"$")	# string? remove quotes and convert to a ;-list
				separate_arguments(value UNIX_COMMAND ${CMAKE_MATCH_1})
			endif()
			set(${key} ${value} PARENT_SCOPE)
		endif()
	endforeach()
endfunction()

#-----------------------------------------------------------------------------------------------------------------------
# load variables from selectedConfiguration.mk
#-----------------------------------------------------------------------------------------------------------------------

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/selectedConfiguration.mk)
	message(FATAL_ERROR "${CMAKE_SOURCE_DIR}/selectedConfiguration.mk file does not exist; Please run first "
	"'make configure [CONFIG_PATH=<path-to-distortosConfiguration.mk>]'")
endif()
# force CMake to rerun if selectedConfiguration.mk file changes
configure_file(${CMAKE_SOURCE_DIR}/selectedConfiguration.mk selectedConfiguration.mk)
loadConfiguration(${CMAKE_SOURCE_DIR}/selectedConfiguration.mk)

#-----------------------------------------------------------------------------------------------------------------------
# load variables from selected distortosConfiguration.mk
#-----------------------------------------------------------------------------------------------------------------------

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/${CONFIG_SELECTED_CONFIGURATION})
	message(FATAL_ERROR "Selected configuration file '${CMAKE_SOURCE_DIR}/${CONFIG_SELECTED_CONFIGURATION}' does not "
	"exist")
endif()
# force CMake to rerun if selected distortosConfiguration.mk file changes
configure_file(${CMAKE_SOURCE_DIR}/${CONFIG_SELECTED_CONFIGURATION} ${CONFIG_SELECTED_CONFIGURATION})
loadConfiguration(${CMAKE_SOURCE_DIR}/${CONFIG_SELECTED_CONFIGURATION})

#-----------------------------------------------------------------------------------------------------------------------
# distortosConfiguration.h
#-----------------------------------------------------------------------------------------------------------------------

add_custom_command(OUTPUT include/distortos/distortosConfiguration.h
		COMMAND ${CMAKE_COMMAND} -E make_directory include/distortos
		COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/scripts/makeDistortosConfiguration.sh
		${CMAKE_SOURCE_DIR}/${CONFIG_SELECTED_CONFIGURATION} > include/distortos/distortosConfiguration.h
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/makeDistortosConfiguration.sh
		DEPENDS ${CMAKE_SOURCE_DIR}/${CONFIG_SELECTED_CONFIGURATION}
		DEPENDS ${CMAKE_SOURCE_DIR}/selectedConfiguration.mk
		VERBATIM
		USES_TERMINAL)
add_custom_target(distortosConfiguration-h ALL DEPENDS include/distortos/distortosConfiguration.h)

#-----------------------------------------------------------------------------------------------------------------------
# distortos-includes interface library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos-includes INTERFACE)
target_include_directories(distortos-includes INTERFACE
		${CMAKE_CURRENT_BINARY_DIR}/include
		include
		${CONFIG_ARCHITECTURE_INCLUDES}
		${CONFIG_CHIP_INCLUDES}
		${CONFIG_BOARD_INCLUDES})
add_dependencies(distortos-includes distortosConfiguration-h)

#-----------------------------------------------------------------------------------------------------------------------
# distortos::includes alias library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos::includes ALIAS distortos-includes)

#-----------------------------------------------------------------------------------------------------------------------
# distortos-baseCFlags interface library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos-baseCFlags INTERFACE)
target_compile_options(distortos-baseCFlags INTERFACE
		${CONFIG_ARCHITECTURE_FLAGS})

#-----------------------------------------------------------------------------------------------------------------------
# distortos::baseCFlags alias library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos::baseCFlags ALIAS distortos-baseCFlags)

#-----------------------------------------------------------------------------------------------------------------------
# distortos-cFlags interface library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos-cFlags INTERFACE)
target_compile_options(distortos-cFlags INTERFACE
		${CONFIG_DEBUGGING_INFORMATION_COMPILATION}
		${CONFIG_BUILD_OPTIMIZATION}
		${CONFIG_LINK_TIME_OPTIMIZATION_COMPILATION}
		-ffunction-sections
		-fdata-sections
		${CONFIG_ASSERT})
target_link_libraries(distortos-cFlags INTERFACE
		distortos::baseCFlags)

#-----------------------------------------------------------------------------------------------------------------------
# distortos::cFlags alias library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos::cFlags ALIAS distortos-cFlags)

#-----------------------------------------------------------------------------------------------------------------------
# distortos-allCFlags interface library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos-allCFlags INTERFACE)
target_compile_options(distortos-allCFlags INTERFACE
		-Wall
		-Wextra
		-Wshadow)
target_link_libraries(distortos-allCFlags INTERFACE
		distortos::cFlags)

#-----------------------------------------------------------------------------------------------------------------------
# distortos::allCFlags alias library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos::allCFlags ALIAS distortos-allCFlags)

#-----------------------------------------------------------------------------------------------------------------------
# distortos-baseCxxFlags interface library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos-baseCxxFlags INTERFACE)
target_compile_options(distortos-baseCxxFlags INTERFACE
		${CONFIG_STATIC_DESTRUCTORS_RUN_TIME_REGISTRATION}
		-fno-rtti
		-fno-exceptions)
target_link_libraries(distortos-baseCxxFlags INTERFACE
		distortos::baseCFlags)

#-----------------------------------------------------------------------------------------------------------------------
# distortos::baseCxxFlags alias library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos::baseCxxFlags ALIAS distortos-baseCxxFlags)

#-----------------------------------------------------------------------------------------------------------------------
# distortos-cxxFlags interface library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos-cxxFlags INTERFACE)
target_link_libraries(distortos-cxxFlags INTERFACE
		distortos::baseCxxFlags
		distortos::cFlags)

#-----------------------------------------------------------------------------------------------------------------------
# distortos::cxxFlags alias library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos::cxxFlags ALIAS distortos-cxxFlags)

#-----------------------------------------------------------------------------------------------------------------------
# distortos-allCxxFlags interface library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos-allCxxFlags INTERFACE)
target_link_libraries(distortos-allCxxFlags INTERFACE
		distortos::cxxFlags
		distortos::allCFlags)

#-----------------------------------------------------------------------------------------------------------------------
# distortos::allCxxFlags alias library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos::allCxxFlags ALIAS distortos-allCxxFlags)

#-----------------------------------------------------------------------------------------------------------------------
# distortos static library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos STATIC "")
set_target_properties(distortos PROPERTIES
		CXX_STANDARD 11
		CXX_STANDARD_REQUIRED ON)
target_link_libraries(distortos PRIVATE
		distortos::allCxxFlags
		distortos::includes)
target_link_libraries(distortos PUBLIC
		littlefs)

include(source/distortos-sources.cmake)

file(GLOB_RECURSE distortosBoardSourcesFiles FOLLOW_SYMLINKS LIST_DIRECTORIES false
		"${CMAKE_SOURCE_DIR}/*/distortos-board-sources.cmake")
foreach(distortosBoardSources ${distortosBoardSourcesFiles})
	include(${distortosBoardSources})
endforeach()

#-----------------------------------------------------------------------------------------------------------------------
# preprocess linker script
#-----------------------------------------------------------------------------------------------------------------------

if(NOT LDSCRIPT)
	if(NOT CONFIG_LDSCRIPT)
		message(FATAL_ERROR "Non-board configurations are not supported")
	endif()
	set(LDSCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_LDSCRIPT}")
endif()
if(NOT EXISTS ${LDSCRIPT})
	message(FATAL_ERROR "Linker script file '${LDSCRIPT}' does not exist")
endif()
get_filename_component(PREPROCESSED_LDSCRIPT ${LDSCRIPT} NAME_WE)
set(PREPROCESSED_LDSCRIPT "${PREPROCESSED_LDSCRIPT}.preprocessed.ld")
set(ENV{DISTORTOS_LINKER_SCRIPT} "${CMAKE_CURRENT_BINARY_DIR}/${PREPROCESSED_LDSCRIPT}")
add_custom_command(OUTPUT ${PREPROCESSED_LDSCRIPT}
		COMMAND ${CMAKE_CXX_COMPILER} -nostdinc -undef -C -E -P -x assembler-with-cpp
		-I${CMAKE_CURRENT_BINARY_DIR}/include ${LDSCRIPT} -o ${PREPROCESSED_LDSCRIPT}
		DEPENDS ${LDSCRIPT}
		DEPENDS distortosConfiguration-h
		DEPENDS include/distortos/distortosConfiguration.h
		VERBATIM
		USES_TERMINAL)
add_custom_target(ldscript ALL DEPENDS ${PREPROCESSED_LDSCRIPT})

#-----------------------------------------------------------------------------------------------------------------------
# distortos-distortos interface library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos-distortos INTERFACE)
target_link_libraries(distortos-distortos INTERFACE
		distortos::baseCxxFlags
		distortos::includes
		${CONFIG_ARCHITECTURE_FLAGS}
		${CONFIG_DEBUGGING_INFORMATION_LINKING}
		${CONFIG_BUILD_OPTIMIZATION}
		${CONFIG_LINK_TIME_OPTIMIZATION_LINKING}
		-Wl,--gc-sections
		-L"${CMAKE_CURRENT_BINARY_DIR}"
		-Wl,--whole-archive
		distortos
		-Wl,--no-whole-archive)
add_dependencies(distortos-distortos ldscript)

#-----------------------------------------------------------------------------------------------------------------------
# distortos::distortos alias library
#-----------------------------------------------------------------------------------------------------------------------

add_library(distortos::distortos ALIAS distortos-distortos)

#-----------------------------------------------------------------------------------------------------------------------
# distortos-doxygen
#-----------------------------------------------------------------------------------------------------------------------

doxygen(INPUT ${CMAKE_CURRENT_SOURCE_DIR}/documentation ${CMAKE_CURRENT_BINARY_DIR}/include
		${CMAKE_CURRENT_SOURCE_DIR}/include
		INCLUDE_PATH ${CMAKE_CURRENT_BINARY_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include)

getTargetPropertyRecursive(doxygenCompileOptions distortos COMPILE_OPTIONS)
# for now C++11 standard with GNU extensions is hardcoded here...
execute_process(COMMAND ${CMAKE_COMMAND} -E echo ""
		COMMAND ${CMAKE_CXX_COMPILER} ${doxygenCompileOptions} -std=gnu++11 -E -P -dD -x c++ -
		OUTPUT_VARIABLE preprocessedOutput)
string(REGEX REPLACE "[\r\n]+" ";" preprocessedOutput "${preprocessedOutput}")
foreach(predefinedEntry ${preprocessedOutput})
	if("${predefinedEntry}" MATCHES "^#define ([^ ]+) (.*)$")
		set(key ${CMAKE_MATCH_1})
		set(value ${CMAKE_MATCH_2})
		string(REPLACE "\"" "\\\"" value "${value}")
		list(APPEND doxygenPredefined "\"${key}=${value}\"")
	endif()
endforeach()

execute_process(COMMAND git describe --dirty
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		RESULT_VARIABLE gitDescribeResult
		OUTPUT_VARIABLE doxygenProjectNumber
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT gitDescribeResult EQUAL 0)
	string(TIMESTAMP doxygenProjectNumber "%Y%m%d%H%M%S")
endif()

foreach(doxygenExcludeEntry $ENV{doxygenExclude})
	string(APPEND doxygenExclude " \"${doxygenExcludeEntry}\"")
endforeach()
foreach(doxygenIncludePathEntry $ENV{doxygenIncludePath})
	string(APPEND doxygenIncludePath " \"${doxygenIncludePathEntry}\"")
endforeach()
foreach(doxygenInputEntry $ENV{doxygenInput})
	string(APPEND doxygenInput " \"${doxygenInputEntry}\"")
endforeach()
set(doxygenStripFromIncludePathList "$ENV{doxygenIncludePath};$ENV{doxygenInput}")
list(REMOVE_DUPLICATES doxygenStripFromIncludePathList)
foreach(doxygenStripFromIncludePathEntry ${doxygenStripFromIncludePathList})
	string(APPEND doxygenStripFromIncludePath " \"${doxygenStripFromIncludePathEntry}\"")
endforeach()
string(REPLACE ";" " " doxygenPredefined "${doxygenPredefined}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.extension
		"EXCLUDE =${doxygenExclude}\n"
		"INCLUDE_PATH =${doxygenIncludePath}\n"
		"INPUT =${doxygenInput}\n"
		"PREDEFINED = DOXYGEN \"__attribute__(x)=\" \"__GNUC_PREREQ(x, y)=1\" ${doxygenPredefined}\n"
		"PROJECT_NUMBER = ${doxygenProjectNumber}\n"
		"STRIP_FROM_INC_PATH =${doxygenStripFromIncludePath}\n"
		"STRIP_FROM_PATH = \"${CMAKE_CURRENT_SOURCE_DIR}\"\n")

set(doxyfiles "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile;${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.extension")
add_custom_target(distortos-doxygen
		${CMAKE_COMMAND} -D "DOXYFILES=${doxyfiles}" -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/doxygen.cmake
		DEPENDS distortosConfiguration-h
		DEPENDS include/distortos/distortosConfiguration.h
		VERBATIM)

#-----------------------------------------------------------------------------------------------------------------------
# distortosTest application
#-----------------------------------------------------------------------------------------------------------------------

add_subdirectory(test)
