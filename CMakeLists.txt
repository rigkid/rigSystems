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
)
target_link_libraries(${PACK_NAME} PUBLIC rigkit rigComponent)
target_include_directories(${PACK_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
)
message(STATUS "Configured pack: ${PACK_NAME} (ECS systems)")
