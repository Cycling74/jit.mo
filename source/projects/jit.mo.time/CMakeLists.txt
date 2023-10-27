
set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)


#############################################################
# MAX EXTERNAL
#############################################################


include_directories( 
	"${C74_INCLUDES}"
	${CMAKE_CURRENT_SOURCE_DIR}/../common
)


add_library( 
	${PROJECT_NAME} 
	MODULE
	${PROJECT_NAME}.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../common/jit.mo.common.h
	${CMAKE_CURRENT_SOURCE_DIR}/../common/jit.mo.singleton.h
	${CMAKE_CURRENT_SOURCE_DIR}/../common/noise1234.c
)


include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)


#############################################################
# UNIT TEST
#############################################################

include(${C74_MIN_API_DIR}/test/min-object-unittest.cmake)
