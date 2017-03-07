set(JAVASCRIPT_ASSETS
    js/selController.js
    js/lodash.core.js
    js/ui-bootstrap-tpls-2.1.3.js
    js/angular-cookies.js
    js/angular-websocket.js
    js/angular-ui-router.js
    js/kvm-controller.js
    js/logincontroller.js
    js/angular-resource.js
    js/angular-sanitize.js
    js/bmcApp.js
    js/angular-animate.js
    js/run_prettify.js
    js/angular.js
    js/versionController.js
    noVNC/utils/parse.js
    noVNC/utils/inflator.partial.js
    noVNC/karma.conf.js
    noVNC/include/inflator.js
    noVNC/include/keysym.js
    noVNC/include/chrome-app/tcp-client.js
    noVNC/include/ui.js
    noVNC/include/keysymdef.js
    noVNC/include/websock.js
    noVNC/include/util.js
    noVNC/include/base64.js
    noVNC/include/logo.js
    noVNC/include/input.js
    noVNC/include/rfb.js
    noVNC/include/des.js
    noVNC/include/playback.js
    noVNC/include/xtscancodes.js
    noVNC/include/keyboard.js
    noVNC/include/webutil.js
    noVNC/include/display.js
)

set(CSS_ASSETS
    css/intel.css
    css/bootstrap.css
    css/font-awesome.css
    css/bootstrap-theme.css
    css/prettify.css
    noVNC/include/base.css
    noVNC/include/blue.css
    noVNC/include/black.css
)

set(HTML_ASSETS
    index.html
    login.html
    partial-eventlog.html
    partial-fruinfo.html
    partial-home-list.html
    partial-kvm.html
    partial-systeminfo.html
    websocket.html
)

set(IMG_ASSETS
    img/logo.png
)

set(STATIC_ASSETS_OUT "")
foreach(JAVASCRIPT_ASSET ${JAVASCRIPT_ASSETS})
    # if it's a debug build, don't minify
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND STATIC_ASSETS_OUT ${CMAKE_CURRENT_SOURCE_DIR}/${JAVASCRIPT_ASSET})
    else()
        set(OUTPUT_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/${JAVASCRIPT_ASSET})
        get_filename_component(FOLDERNAME ${OUTPUT_FILENAME} DIRECTORY)
        file(MAKE_DIRECTORY "${FOLDERNAME}")
        # TODO only minify if not a debug build
        add_custom_command(OUTPUT ${OUTPUT_FILENAME} 
            COMMAND closure-compiler --language_in=ECMASCRIPT5
            --js  "${CMAKE_CURRENT_SOURCE_DIR}/${JAVASCRIPT_ASSET}"
            --js_output_file "${CMAKE_CURRENT_BINARY_DIR}/${JAVASCRIPT_ASSET}"

            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${JAVASCRIPT_ASSET}"
            COMMENT "Minifying ${JAVASCRIPT_ASSET}"
        )
        list(APPEND STATIC_ASSETS_OUT ${CMAKE_CURRENT_BINARY_DIR}/${JAVASCRIPT_ASSET})
    endif (CMAKE_BUILD_TYPE STREQUAL "Debug")
endforeach(JAVASCRIPT_ASSET)

# for now CSS is included as is
foreach(CSS_ASSET ${CSS_ASSETS})
    list(APPEND STATIC_ASSETS_OUT ${CMAKE_CURRENT_SOURCE_DIR}/${CSS_ASSET})
endforeach(CSS_ASSET)

# for now HTML is included as is
foreach(HTML_ASSET ${HTML_ASSETS})
    list(APPEND STATIC_ASSETS_OUT ${CMAKE_CURRENT_SOURCE_DIR}/${HTML_ASSET})
endforeach(HTML_ASSET)

# for now IMG is included as is
foreach(IMG_ASSET ${IMG_ASSETS})
    list(APPEND STATIC_ASSETS_OUT ${CMAKE_CURRENT_SOURCE_DIR}/${IMG_ASSET})
endforeach(IMG_ASSET)

add_custom_target(minifyjs ALL DEPENDS ${STATIC_ASSETS_OUT})

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)

set(CXX_STATIC_ASSETS_OUTPUT_FILE ${CMAKE_BINARY_DIR}/generated/webassets.cpp)
set(WEBASSET_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/build_web_assets.py")

add_custom_command(OUTPUT ${CXX_STATIC_ASSETS_OUTPUT_FILE}
    COMMAND ${WEBASSET_SCRIPT} 
        -o ${CXX_STATIC_ASSETS_OUTPUT_FILE} -i ${STATIC_ASSETS_OUT}

    DEPENDS ${STATIC_ASSETS_OUT} ${WEBASSET_SCRIPT}
    COMMENT "Building CPP file ${CXX_STATIC_ASSETS_OUTPUT_FILE}"
)

add_custom_target(packagestaticcpp ALL DEPENDS ${CXX_STATIC_ASSETS_OUTPUT_FILE})