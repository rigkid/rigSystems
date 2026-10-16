file(READ "${CMAKE_CURRENT_LIST_DIR}/pack.json" PACK_JSON)
string(JSON PACK_NAME GET "${PACK_JSON}" name)

# Ensure data pack is available (local checkout preferred).
if(NOT TARGET rigComponent)
	if(DEFINED RIGKIT_ROOT AND NOT RIGKIT_ROOT STREQUAL "")
		set(_rigComponent_dir "${RIGKIT_ROOT}/packs/rigComponent")
	else()
		set(_rigComponent_dir "${CMAKE_SOURCE_DIR}/packs/rigComponent")
	endif()
	if(EXISTS "${_rigComponent_dir}/CMakeLists.txt")
		add_subdirectory("${_rigComponent_dir}" "${CMAKE_BINARY_DIR}/packs/rigComponent")
	else()
		message(FATAL_ERROR "rigSystems requires rigComponent under packs/rigComponent")
	endif()
endif()

if(NOT TARGET rigProject)
	if(DEFINED RIGKIT_ROOT AND NOT RIGKIT_ROOT STREQUAL "")
		set(_rigProject_dir "${RIGKIT_ROOT}/packs/rigProject")
	else()
		set(_rigProject_dir "${CMAKE_SOURCE_DIR}/packs/rigProject")
	endif()
	if(EXISTS "${_rigProject_dir}/CMakeLists.txt")
		add_subdirectory("${_rigProject_dir}" "${CMAKE_BINARY_DIR}/packs/rigProject")
	else()
		message(FATAL_ERROR "rigSystems requires rigProject under packs/rigProject")
	endif()
endif()

if(NOT TARGET rigAnimComponent)
	if(DEFINED RIGKIT_ROOT AND NOT RIGKIT_ROOT STREQUAL "")
		set(_rigAnimComponent_dir "${RIGKIT_ROOT}/packs/rigAnimComponent")
	else()
		set(_rigAnimComponent_dir "${CMAKE_SOURCE_DIR}/packs/rigAnimComponent")
	endif()
	if(EXISTS "${_rigAnimComponent_dir}/CMakeLists.txt")
		add_subdirectory("${_rigAnimComponent_dir}" "${CMAKE_BINARY_DIR}/packs/rigAnimComponent")
	else()
		message(FATAL_ERROR "rigSystems requires rigAnimComponent under packs/rigAnimComponent")
	endif()
endif()

add_library(${PACK_NAME} STATIC
	src/rigSystems.cpp
	src/rigSystems.h
	src/SCanvas.cpp
	src/SCanvas.h
	src/SHierarchy.cpp
	src/SHierarchy.h
	src/SModulators.cpp
	src/SModulators.h
	src/SOrbitDrive.cpp
	src/SOrbitDrive.h
	src/OrbitNav.cpp
	src/OrbitNav.h
	src/STweens.cpp
	src/STweens.h
	src/SShapeRendering.cpp
	src/SShapeRendering.h
	src/ComponentSerializers.cpp
	src/ComponentSerializers.h
	src/AnimSerializers.cpp
	src/AnimSerializers.h
)
target_link_libraries(${PACK_NAME} PUBLIC rigkit rigComponent rigAnimComponent)
if(TARGET rigProject)
	target_link_libraries(${PACK_NAME} PUBLIC rigProject)
endif()
target_include_directories(${PACK_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
)
if(MSVC)
	target_compile_options(${PACK_NAME} PRIVATE /bigobj)
elseif(MINGW OR (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
	target_compile_options(${PACK_NAME} PRIVATE -Wa,-mbig-obj)
elseif(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(${PACK_NAME} PRIVATE -bigobj)
endif()
message(STATUS "Configured pack: ${PACK_NAME} (ECS systems)")
