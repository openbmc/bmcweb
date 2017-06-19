set(JAVASCRIPT_ASSETS
    js/selController.js
    js/lodash.core.js
    js/ui-bootstrap-tpls-2.5.0.js
    js/angular-cookies.js
    js/angular-websocket.js
    js/angular-ui-router.js
    js/kvmController.js
    js/loginController.js
    js/ipmiController.js
    js/fwupdateController.js
    js/mainController.js
    js/versionController.js
    js/sensorController.js
    js/angular-sanitize.js
    js/bmcApp.js
    js/base64.js
    js/angular-animate.js
    js/run_prettify.js
    js/angular.js
    js/angular-ui-router-uib-modal.js
    
    noVNC/core/inflator.js
    noVNC/core/input/xtscancodes.js
    noVNC/core/input/util.js
    noVNC/core/input/devices.js
    noVNC/core/input/keysym.js
    noVNC/core/input/keysymdef.js
    noVNC/core/websock.js
    noVNC/core/util.js
    noVNC/core/base64.js
    noVNC/core/rfb.js
    noVNC/core/des.js
    noVNC/core/display.js
    noVNC/app/ui.js
    noVNC/app/webutil.js

)

set(CSS_ASSETS
    css/intel.css
    css/bootstrap.css
    css/font-awesome.css
    css/bootstrap-theme.css
    css/prettify.css
    noVNC/app/styles/base.css
    noVNC/app/styles/auto.css
)

set(HTML_ASSETS
    index.html
    partial-login.html
    partial-eventlog.html
    partial-fruinfo.html
    partial-home-list.html
    partial-ipmi.html
    partial-kvm.html
    partial-sensor.html
    partial-systeminfo.html
    partial-fwupdate.html
    partial-fwupdateconfirm.html
)

set(OTHER_ASSETS
    img/logo.png
    img/blur-bg.jpg
    fonts/fontawesome-webfont.woff
)

set(STATIC_ASSETS_OUT "")
set(MINIFIED_ASSETS_OUT "")
find_program(CSS_MINIFIER uglifyjs)
if(NOT UGLIFY_MINIFIER)
    message("uglifyjs not found")
endif()
foreach(JAVASCRIPT_ASSET ${JAVASCRIPT_ASSETS})

    set(MINIFIED_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/${JAVASCRIPT_ASSET})
    get_filename_component(FOLDERNAME ${MINIFIED_FILENAME} DIRECTORY)
    # string(REGEX REPLACE "(\\.[^.]*$)" ".min\\1" OUTPUT_FILENAME ${OUTPUT_FILENAME})
    file(MAKE_DIRECTORY "${FOLDERNAME}")
    add_custom_command(OUTPUT ${MINIFIED_FILENAME} 
        COMMAND uglifyjs --compress --mangle  
        --output "${MINIFIED_FILENAME}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${JAVASCRIPT_ASSET}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${JAVASCRIPT_ASSET}"
        COMMENT "Minifying ${JAVASCRIPT_ASSET}"
    )

    list(APPEND MINIFIED_ASSETS_OUT ${MINIFIED_FILENAME})
    # if it's a debug build, use the unminified version
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT UGLIFY_MINIFIER)
        list(APPEND STATIC_ASSETS_OUT ${CMAKE_CURRENT_SOURCE_DIR}/${JAVASCRIPT_ASSET})
    else()
        list(APPEND STATIC_ASSETS_OUT ${MINIFIED_FILENAME})
    endif (CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT UGLIFY_MINIFIER)
endforeach(JAVASCRIPT_ASSET)


find_program(CSS_MINIFIER cssnano)
if(NOT CSS_MINIFIER)
    message("cssnano not found")
endif()
# for now CSS is included as is
foreach(CSS_ASSET ${CSS_ASSETS})
    set(MINIFIED_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/${CSS_ASSET})
    get_filename_component(FOLDERNAME ${MINIFIED_FILENAME} DIRECTORY)
    file(MAKE_DIRECTORY "${FOLDERNAME}")
    if(CSS_MINIFIER)   
        add_custom_command(OUTPUT ${MINIFIED_FILENAME} 
            COMMAND cssnano 
            "${CMAKE_CURRENT_SOURCE_DIR}/${CSS_ASSET}"
            "${CMAKE_CURRENT_BINARY_DIR}/${CSS_ASSET}"

            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${CSS_ASSET}"
            COMMENT "Minifying ${CSS_ASSET}"
        )
        list(APPEND MINIFIED_ASSETS_OUT ${MINIFIED_FILENAME})
    endif()
    # if it's a debug build, use the unminified version
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT CSS_MINIFIER)
        list(APPEND STATIC_ASSETS_OUT ${CMAKE_CURRENT_SOURCE_DIR}/${CSS_ASSET})
    else()
        list(APPEND STATIC_ASSETS_OUT ${MINIFIED_FILENAME})
    endif (CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT CSS_MINIFIER)
    
endforeach(CSS_ASSET)

# for now HTML is included as is
foreach(HTML_ASSET ${HTML_ASSETS})
    list(APPEND STATIC_ASSETS_OUT ${CMAKE_CURRENT_SOURCE_DIR}/${HTML_ASSET})
endforeach(HTML_ASSET)

# for now IMG is included as is
foreach(OTHER_ASSET ${OTHER_ASSETS})
    list(APPEND STATIC_ASSETS_OUT ${CMAKE_CURRENT_SOURCE_DIR}/${OTHER_ASSET})
endforeach(OTHER_ASSET)

# this taret is the dependency that the build script uses to make sure everything is available
add_custom_target(buildstatic ALL DEPENDS ${STATIC_ASSETS_OUT})

# this target makes sure that assets are minified in debug builds, even if they aren't used
add_custom_target(minifyassets ALL DEPENDS ${MINIFIED_ASSETS_OUT})


file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)

set(CXX_STATIC_ASSETS_OUTPUT_FILE ${CMAKE_BINARY_DIR}/generated/webassets.cpp)
set(WEBASSET_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/build_web_assets.py")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEBUG_MODE "-d")
endif()

add_custom_command(OUTPUT ${CXX_STATIC_ASSETS_OUTPUT_FILE}
    COMMAND ${WEBASSET_SCRIPT} ${DEBUG_MODE}
        -o ${CXX_STATIC_ASSETS_OUTPUT_FILE} -i ${STATIC_ASSETS_OUT}

    DEPENDS ${STATIC_ASSETS_OUT} ${WEBASSET_SCRIPT}
    COMMENT "Building CPP file ${CXX_STATIC_ASSETS_OUTPUT_FILE}"
)

add_custom_target(packagestaticcpp ALL DEPENDS ${CXX_STATIC_ASSETS_OUTPUT_FILE})