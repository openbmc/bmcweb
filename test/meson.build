
srcfiles_unittest = files(
    'http/crow_getroutes_test.cpp',
    'http/http2_connection_test.cpp',
    'http/http_body_test.cpp',
    'http/http_connection_test.cpp',
    'http/http_response_test.cpp',
    'http/mutual_tls.cpp',
    'http/parsing_test.cpp',
    'http/router_test.cpp',
    'http/server_sent_event_test.cpp',
    'http/utility_test.cpp',
    'http/verb_test.cpp',
    'http/zstd_decompressor_test.cpp',
    'include/async_resolve_test.cpp',
    'include/authentication.cpp',
    'include/credential_pipe_test.cpp',
    'include/dbus_privileges_test.cpp',
    'include/http_utility_test.cpp',
    'include/human_sort_test.cpp',
    'include/json_html_serializer.cpp',
    'include/multipart_test.cpp',
    'include/ossl_random.cpp',
    'include/sessions_test.cpp',
    'include/ssl_key_handler_test.cpp',
    'include/str_utility_test.cpp',
    'redfish-core/include/dbus_log_watcher_test.cpp',
    'redfish-core/include/event_log_test.cpp',
    'redfish-core/include/event_matches_filter_test.cpp',
    'redfish-core/include/filter_expr_executor_test.cpp',
    'redfish-core/include/filter_expr_parser_test.cpp',
    'redfish-core/include/privileges_test.cpp',
    'redfish-core/include/redfish_aggregator_test.cpp',
    'redfish-core/include/redfish_oem_routing_test.cpp',
    'redfish-core/include/redfish_test.cpp',
    'redfish-core/include/registries_test.cpp',
    'redfish-core/include/submit_test_event_test.cpp',
    'redfish-core/include/utils/dbus_utils.cpp',
    'redfish-core/include/utils/error_code_test.cpp',
    'redfish-core/include/utils/hex_utils_test.cpp',
    'redfish-core/include/utils/ip_utils_test.cpp',
    'redfish-core/include/utils/json_utils_test.cpp',
    'redfish-core/include/utils/query_param_test.cpp',
    'redfish-core/include/utils/sensor_utils_test.cpp',
    'redfish-core/include/utils/stl_utils_test.cpp',
    'redfish-core/include/utils/systems_utils_test.cpp',
    'redfish-core/include/utils/time_utils_test.cpp',
    'redfish-core/lib/chassis_test.cpp',
    'redfish-core/lib/ethernet_test.cpp',
    'redfish-core/lib/log_services_dump_test.cpp',
    'redfish-core/lib/manager_diagnostic_data_test.cpp',
    'redfish-core/lib/metadata_test.cpp',
    'redfish-core/lib/power_subsystem_test.cpp',
    'redfish-core/lib/service_root_test.cpp',
    'redfish-core/lib/system_test.cpp',
    'redfish-core/lib/systems_logservices_postcode.cpp',
    'redfish-core/lib/thermal_subsystem_test.cpp',
    'redfish-core/lib/update_service_test.cpp',
) + test_sources

if (get_option('tests').allowed())
    gtest = dependency(
        'gtest_main',
        main: true,
        version: '>=1.15.0',
        required: true,
    )
    gmock = dependency('gmock', required: true)
    gtestlib = static_library('gtestlib', dependencies: [gtest, gmock])
    gtestdep = declare_dependency(
        link_with: gtestlib,
        dependencies: [
            gtest.partial_dependency(includes: true),
            gmock.partial_dependency(includes: true),
        ],
    )
    # generate the test executable
    foreach test_src : srcfiles_unittest
        test_bin = executable(
            fs.stem(test_src),
            test_src,
            link_with: bmcweblib,
            include_directories: [incdir, include_directories('..')],
            install_dir: bindir,
            dependencies: bmcweb_dependencies + [gtestdep],
        )
        test(fs.stem(test_src), test_bin, protocol: 'gtest')
    endforeach
endif
