# Merge multiple static archives into one.
# Usage: cmake -DINPUT="a1.a;a2.a;..." -DOUTPUT="out.a" -DAR_TOOL="/usr/bin/ar" -P merge_archives.cmake

set(TMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/_merge_tmp_$ENV{RANDOM}")
file(REMOVE_RECURSE "${TMP_DIR}")
file(MAKE_DIRECTORY "${TMP_DIR}")

separate_arguments(ARCHIVE_LIST NATIVE_COMMAND "${INPUT}")

foreach(archive IN LISTS ARCHIVE_LIST)
    execute_process(
        COMMAND ${AR_TOOL} x "${archive}"
        WORKING_DIRECTORY "${TMP_DIR}"
        RESULT_VARIABLE res
    )
    if(NOT res EQUAL 0)
        message(FATAL_ERROR "Failed to extract ${archive}")
    endif()
endforeach()

file(GLOB OBJ_FILES LIST_DIRECTORIES false "${TMP_DIR}/*.o")
if(NOT OBJ_FILES)
    message(FATAL_ERROR "No object files extracted")
endif()

execute_process(
    COMMAND ${AR_TOOL} rcs "${OUTPUT}" ${OBJ_FILES}
    RESULT_VARIABLE res
)
if(NOT res EQUAL 0)
    message(FATAL_ERROR "Failed to create merged archive ${OUTPUT}")
endif()

file(REMOVE_RECURSE "${TMP_DIR}")
message(STATUS "Merged archive: ${OUTPUT}")
