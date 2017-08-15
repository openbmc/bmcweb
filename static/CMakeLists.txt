set(TIMESTAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/webpack.timestamp")


# this could be improved.  It basically sets all source files of a certain type as dependencies
execute_process(
    COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR}/phosphor-webui ls-files --full-name 
    *.js *.css *.html
    OUTPUT_VARIABLE PHOSPHOR_FILES
    )
STRING(REGEX REPLACE "\n" ";" PHOSPHOR_FILES "${PHOSPHOR_FILES}")
    
SET(PHOSPHOR_FILES_FULLPATH "")
FOREACH(filename ${PHOSPHOR_FILES})
    LIST(APPEND PHOSPHOR_FILES_FULLPATH "${CMAKE_CURRENT_SOURCE_DIR}/phosphor-webui/${filename}")
ENDFOREACH(filename ${PHOSPHOR_FILES})

# if this is a debug build, don't minify
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(DEBUG_ADD --devtool source-map)
ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

add_custom_command(
    COMMAND
    cd ${CMAKE_CURRENT_SOURCE_DIR}/phosphor-webui &&
    npm install yarn --no-progress --loglevel info -g --prefix ${CMAKE_BINARY_DIR}/node_prefix &&
    ${CMAKE_BINARY_DIR}/node_prefix/bin/yarn install --dev --no-progress --loglevel info --network-concurrency 1 &&
    ${CMAKE_BINARY_DIR}/node_prefix/bin/yarn run build ${DEBUG_ADD} && 
    touch ${TIMESTAMP_FILE}
    OUTPUT ${TIMESTAMP_FILE}
    DEPENDS ${PHOSPHOR_FILES_FULLPATH}
    )

add_custom_target(webpackbuild 
    DEPENDS ${TIMESTAMP_FILE}
    SOURCES ${PHOSPHOR_FILES_FULLPATH}
    )

set(WEBASSET_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/build_web_assets.py")
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated/webassets.cpp
    OUTPUT ${CMAKE_BINARY_DIR}/generated/webassets.hpp
    COMMAND python3 ${WEBASSET_SCRIPT}
        -o ${CMAKE_BINARY_DIR}/generated/webassets.cpp 
        -i ${CMAKE_CURRENT_SOURCE_DIR}/phosphor-webui/dist
    DEPENDS ${TIMESTAMP_FILE} ${WEBASSET_SCRIPT}
    COMMENT "Building CPP file webassets.cpp"
)

add_custom_target(packagestaticcpp ALL DEPENDS 
    ${CMAKE_BINARY_DIR}/generated/webassets.cpp
    ${CMAKE_BINARY_DIR}/generated/webassets.hpp
)
add_dependencies(packagestaticcpp webpackbuild)
