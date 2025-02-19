include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

set(PROJ_NAME prosper)
pr_add_library(${PROJ_NAME} SHARED)

pr_add_dependency(${PROJ_NAME} util_image TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} iglfw TARGET PUBLIC)
pr_add_dependency(${PROJ_NAME} util_udm TARGET)
pr_add_dependency(${PROJ_NAME} muparserx TARGET)
pr_add_dependency(${PROJ_NAME} lz4 TARGET)
pr_add_external_dependency(${PROJ_NAME} gli HEADER_ONLY)

pr_add_headers(${PROJ_NAME} "include/")
pr_add_sources(${PROJ_NAME} "src/")

pr_add_compile_definitions(
	${PROJ_NAME}
		-DSHPROSPER_DLL
		-DUIMG_ENABLE_NVTT
)

option(ENABLE_DEBUG_API_DUMP "Enable debug API dump" OFF)

if(ENABLE_DEBUG_API_DUMP)
	pr_add_compile_definitions(
		${PROJ_NAME}
			-DPR_DEBUG_API_DUMP
		PUBLIC
	)
endif()

pr_finalize(${PROJ_NAME})
