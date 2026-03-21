
execute_process(
    COMMAND git describe --tags
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE gitVersionRaw
    RESULT_VARIABLE gitDescribeResult
)
string(STRIP "${gitVersionRaw}" gitVersion)
if (NOT gitDescribeResult EQUAL 0 OR "${gitVersion}" STREQUAL "")
    set(gitVersion "v0.0.1-alpha")
endif()

file(MAKE_DIRECTORY "${RPSB_PLUGINFILE_OUTPUT_DIR}")
set(outputFile "${RPSB_PLUGINFILE_OUTPUT_DIR}/pulse_deck_ts3_${gitVersion}.ts3_plugin")

message("Creating final plugin package in ${RPSB_PLUGINFILE_OUTPUT_DIR}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar "cfv" "${outputFile}" --format=zip .
    WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
)
