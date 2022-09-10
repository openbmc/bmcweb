gtest = dependency('gtest', main: true,disabler: true, required : false)
gmock = dependency('gmock', required : false)
if not gtest.found() and get_option('tests').enabled()
  gtest_proj = subproject('gtest', required: true)
  gtest = gtest_proj.get_variable('gtest_main_dep')
  gmock = gtest_proj.get_variable('gmock_dep')
endif
gtest = gtest.as_system('system')
gmock = gmock.as_system('system')

srcfiles_unittest = [
  'http/crow_getroutes_test.cpp',
  'http/router_test.cpp',
  'http/utility_test.cpp',
  'include/dbus_utility_test.cpp',
  'include/google/google_service_root_test.cpp',
  'include/http_utility_test.cpp',
  'include/human_sort_test.cpp',
  'include/ibm/configfile_test.cpp',
  'include/ibm/lock_test.cpp',
  'include/multipart_test.cpp',
  'include/openbmc_dbus_rest_test.cpp',
  'redfish-core/include/privileges_test.cpp',
  'redfish-core/include/registries_test.cpp',
  'redfish-core/include/utils/hex_utils_test.cpp',
  'redfish-core/include/utils/ip_utils_test.cpp',
  'redfish-core/include/utils/json_utils_test.cpp',
  'redfish-core/include/utils/query_param_test.cpp',
  'redfish-core/include/utils/stl_utils_test.cpp',
  'redfish-core/include/utils/time_utils_test.cpp',
  'redfish-core/lib/chassis_test.cpp',
  'redfish-core/lib/log_services_dump_test.cpp',
  'redfish-core/lib/service_root_test.cpp',
  'redfish-core/lib/thermal_subsystem_test.cpp',
]

foreach src : srcfiles_bmcweb
  srcfiles_unittest += '../' / src
endforeach

ut_bin = executable(
  'bmcweb_unit_test',
  srcfiles_unittest,
  include_directories : incdir,
  install_dir: bindir,
  dependencies: bmcweb_dependencies + [
    gtest,
    gmock,
  ]
)

test('bmcweb_unit_test', ut_bin)
