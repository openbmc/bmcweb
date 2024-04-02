project('bmcweb', 'cpp',
        version : '1.0',
        meson_version: '>=0.63.0',
        default_options: [
            'b_lto_mode=default',
            'b_lto_threads=0',
            'b_lto=true',
            'b_ndebug=if-release',
            'buildtype=debugoptimized',
            'cpp_rtti=false',
            'cpp_std=c++20',
            'warning_level=3',
            'werror=true',
           ])

# Project related links

project_pretty_name = 'bmcweb'
project_url = 'https://github.com/openbmc/' + project_pretty_name
project_issues_url = project_url + '/issues/new'
summary('Issues',project_issues_url, section: 'Report Issues')

# Validate the c++ Standard

if get_option('cpp_std') != 'c++20'
    error('This project requires c++20 support')
endif

# Get compiler and default build type

cxx = meson.get_compiler('cpp')
build = get_option('buildtype')
optimization = get_option('optimization')
summary('Build Type',build, section : 'Build Info')
summary('Optimization',optimization, section : 'Build Info')


# remove debug information for minsize buildtype
if(get_option('buildtype') == 'minsize')
  add_project_arguments(['-fdata-sections', '-ffunction-sections'], language : 'cpp')
  add_project_arguments('-DNDEBUG', language : 'cpp')
endif

if(get_option('dns-resolver') == 'systemd-dbus')
  add_project_arguments('-DBMCWEB_DBUS_DNS_RESOLVER', language : 'cpp')
endif

# Disable lto when compiling with no optimization
if(get_option('optimization') == '0')
  add_project_arguments('-fno-lto', language: 'cpp')
  message('Disabling lto & its supported features as optimization is disabled')
endif

# Include Directories

incdir = include_directories(
  'include',
  'redfish-core/include',
  'redfish-core/lib',
  'http'
)

# Get the options and enable the respective features
## create a MAP of  "options : feature_flag"

feature_map = {
  'basic-auth'                                  : '-DBMCWEB_ENABLE_BASIC_AUTHENTICATION',
  'cookie-auth'                                 : '-DBMCWEB_ENABLE_COOKIE_AUTHENTICATION',
  'google-api'                                  : '-DBMCWEB_ENABLE_GOOGLE_API',
  'host-serial-socket'                          : '-DBMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET',
  'ibm-management-console'                      : '-DBMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE',
  'insecure-disable-auth'                       : '-DBMCWEB_INSECURE_DISABLE_AUTHX',
  'ibm-usb-code-update'                         : '-DBMCWEB_ENABLE_IBM_USB_CODE_UPDATE',
  'insecure-disable-csrf'                       : '-DBMCWEB_INSECURE_DISABLE_CSRF_PREVENTION',
  'insecure-disable-ssl'                        : '-DBMCWEB_INSECURE_DISABLE_SSL',
  'insecure-push-style-notification'            : '-DBMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING',
  'insecure-tftp-update'                        : '-DBMCWEB_INSECURE_ENABLE_REDFISH_FW_TFTP_UPDATE',
  'insecure-ignore-content-type'                : '-DBMCWEB_INSECURE_IGNORE_CONTENT_TYPE',
  'kvm'                                         : '-DBMCWEB_ENABLE_KVM' ,
  'mutual-tls-auth'                             : '-DBMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION',
  'redfish-aggregation'                         : '-DBMCWEB_ENABLE_REDFISH_AGGREGATION',
  'redfish-allow-deprecated-power-thermal'      : '-DBMCWEB_ALLOW_DEPRECATED_POWER_THERMAL',
  'redfish-bmc-journal'                         : '-DBMCWEB_ENABLE_REDFISH_BMC_JOURNAL',
  'redfish-cpu-log'                             : '-DBMCWEB_ENABLE_REDFISH_CPU_LOG',
  'redfish-dbus-log'                            : '-DBMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES',
  'redfish-dump-log'                            : '-DBMCWEB_ENABLE_REDFISH_DUMP_LOG',
  'redfish-host-logger'                         : '-DBMCWEB_ENABLE_REDFISH_HOST_LOGGER',
  'redfish-new-powersubsystem-thermalsubsystem' : '-DBMCWEB_NEW_POWERSUBSYSTEM_THERMALSUBSYSTEM',
  'redfish-oem-manager-fan-data'                : '-DBMCWEB_ENABLE_REDFISH_OEM_MANAGER_FAN_DATA',
  'redfish-provisioning-feature'                : '-DBMCWEB_ENABLE_REDFISH_PROVISIONING_FEATURE',
  'redfish'                                     : '-DBMCWEB_ENABLE_REDFISH',
  'rest'                                        : '-DBMCWEB_ENABLE_DBUS_REST',
  'session-auth'                                : '-DBMCWEB_ENABLE_SESSION_AUTHENTICATION',
  'static-hosting'                              : '-DBMCWEB_ENABLE_STATIC_HOSTING',
  'experimental-redfish-multi-computer-system'  : '-DBMCWEB_ENABLE_MULTI_COMPUTERSYSTEM',
  'vm-websocket'                                : '-DBMCWEB_ENABLE_VM_WEBSOCKET',
  'xtoken-auth'                                 : '-DBMCWEB_ENABLE_XTOKEN_AUTHENTICATION',
  'audit-events'                                : '-DBMCWEB_ENABLE_LINUX_AUDIT_EVENTS',
  #'vm-nbdproxy'                                : '-DBMCWEB_ENABLE_VM_NBDPROXY',
  'ibm-led-extensions'                          : '-DBMCWEB_ENABLE_IBM_LED_EXTENSIONS',
}

# Get the options status and build a project summary to show which flags are
# being enabled during the configuration time.

foreach option_key,option_value : feature_map
  if(get_option(option_key).allowed())
    if(option_key == 'mutual-tls-auth' or option_key == 'insecure-disable-ssl')
      if(get_option('insecure-disable-ssl').disabled() or get_option('mutual-tls-auth').disabled())
        add_project_arguments(option_value,language:'cpp')
        summary(option_key,option_value, section : 'Enabled Features')
      endif
    elif (option_key in ['basic-auth', 'cookie-auth', 'session-auth', 'xtoken-auth', 'mutual-tls-auth'])
      if (get_option('insecure-disable-auth').disabled())
        add_project_arguments(option_value, language:'cpp')
        summary(option_key,option_value, section : 'Enabled Features')
      endif
    else
      summary(option_key,option_value, section : 'Enabled Features')
      add_project_arguments(option_value,language:'cpp')
    endif
  else
      if(option_key == 'insecure-disable-ssl')
        summary('ssl','-DBMCWEB_ENABLE_SSL', section : 'Enabled Features')
        add_project_arguments('-DBMCWEB_ENABLE_SSL', language : 'cpp')
      endif
  endif
endforeach

if(get_option('tests').allowed())
  summary('unittest','NA', section : 'Enabled Features')
endif

# Add compiler arguments

# -Wpedantic, -Wextra comes by default with warning level
add_project_arguments(
  cxx.get_supported_arguments([
  '-Wcast-align',
  '-Wconversion',
  '-Wformat=2',
  '-Wold-style-cast',
  '-Woverloaded-virtual',
  '-Wsign-conversion',
  '-Wunused',
  '-Wno-attributes',
  ]),
  language: 'cpp'
)

if (cxx.get_id() == 'clang' and cxx.version().version_compare('>9.0'))
add_project_arguments(
  cxx.get_supported_arguments([
    '-Weverything',
    '-Wno-c++98-compat-pedantic',
    '-Wno-c++98-compat',
    '-Wno-documentation-unknown-command',
    '-Wno-documentation',
    '-Wno-exit-time-destructors',
    '-Wno-global-constructors',
    '-Wno-newline-eof',
    '-Wno-padded',
    '-Wno-shadow',
    '-Wno-used-but-marked-unused',
    '-Wno-weak-vtables',
    '-Wno-switch-enum',
    '-Wno-unused-macros',
  ]),
  language:'cpp')
endif

# if compiler is gnu-gcc , and version is > 8.0 then we add few more
# compiler arguments , we know that will pass

if (cxx.get_id() == 'gcc' and cxx.version().version_compare('>8.0'))
  add_project_arguments(
    cxx.get_supported_arguments([
     '-Wduplicated-cond',
     '-Wduplicated-branches',
     '-Wlogical-op',
     '-Wnull-dereference',
     '-Wunused-parameter',
     '-Wdouble-promotion',
     '-Wshadow',
     '-Wno-psabi',
     ]),
    language:'cpp')
endif

if (get_option('buildtype') != 'plain')
  if (get_option('b_lto') == true and get_option('optimization')!='0')
    # Reduce the binary size by removing unnecessary
    # dynamic symbol table entries

    add_project_arguments(
     cxx.get_supported_arguments([
     '-fno-fat-lto-objects',
     '-fvisibility=hidden',
     '-fvisibility-inlines-hidden'
     ]),
     language: 'cpp')

    if cxx.has_link_argument('-Wl,--exclude-libs,ALL')
      add_project_link_arguments('-Wl,--exclude-libs,ALL', language: 'cpp')
    endif
  endif
endif

if( get_option('bmcweb-logging') != 'disabled' or \
    get_option('buildtype').startswith('debug'))
 add_project_arguments([
   '-DBMCWEB_ENABLE_DEBUG'
   ],
  language : 'cpp')

  summary('debug', '-DBMCWEB_ENABLE_DEBUG', section : 'Enabled Features')
endif

# Set Compiler Security flags

security_flags = [
  '-fstack-protector-strong',
  '-fPIE',
  '-fPIC',
  '-D_FORTIFY_SOURCE=2',
  '-Wformat',
  '-Wformat-security'
]

## Add security flags for builds of type 'release','debugoptimized' and 'minsize'

if not (get_option('buildtype') == 'plain' or get_option('buildtype').startswith('debug'))
  add_project_arguments(
   cxx.get_supported_arguments([
    security_flags
  ]),
  language: 'cpp')
endif

# Boost dependency configuration

add_project_arguments(
cxx.get_supported_arguments([
  '-DBOOST_ASIO_DISABLE_CONCEPTS',
  '-DBOOST_ALL_NO_LIB',
  '-DBOOST_ALLOW_DEPRECATED_HEADERS',
  '-DBOOST_ASIO_DISABLE_THREADS',
  '-DBOOST_ASIO_NO_DEPRECATED',
  '-DBOOST_ASIO_SEPARATE_COMPILATION',
  '-DBOOST_BEAST_SEPARATE_COMPILATION',
  '-DBOOST_EXCEPTION_DISABLE',
  '-DBOOST_NO_EXCEPTIONS',
  '-DBOOST_URL_NO_SOURCE_LOCATION',
  '-DJSON_NOEXCEPTION',
  '-DOPENSSL_NO_FILENAMES',
  '-DSDBUSPLUS_DISABLE_BOOST_COROUTINES',
]),
language : 'cpp')

# Find the dependency modules, if not found use meson wrap to get them
# automatically during the configure step
bmcweb_dependencies = []

pam = cxx.find_library('pam', required: true)
atomic =  cxx.find_library('atomic', required: true)
openssl = dependency('openssl', required : true)
bmcweb_dependencies += [pam, atomic, openssl]

nghttp2 = dependency('libnghttp2', version: '>=1.52.0', required : false)
if not nghttp2.found()
  cmake = import('cmake')
  opt_var = cmake.subproject_options()
  opt_var.add_cmake_defines({
    'ENABLE_LIB_ONLY': true,
    'ENABLE_STATIC_LIB': true
  })
  nghttp2_ex = cmake.subproject('nghttp2', options: opt_var)
  nghttp2 = nghttp2_ex.dependency('nghttp2')
endif
bmcweb_dependencies += nghttp2

if(get_option('audit-events').allowed())
  audit = cxx.find_library('libaudit', required: true)
  bmcweb_dependencies += audit
endif

sdbusplus = dependency('sdbusplus', required : false, include_type: 'system')
if not sdbusplus.found()
  sdbusplus_proj = subproject('sdbusplus', required: true)
  sdbusplus = sdbusplus_proj.get_variable('sdbusplus_dep')
  sdbusplus = sdbusplus.as_system('system')
endif
bmcweb_dependencies += sdbusplus

tinyxml = dependency(
    'tinyxml2',
    include_type: 'system',
    version: '>=9.0.0',
)
if not tinyxml.found()
    tinyxml_proj = subproject('tinyxml2', required: true)
    tinyxml = tinyxml_proj.get_variable('tinyxml_dep')
    tinyxml = tinyxml.as_system('system')
endif
bmcweb_dependencies += tinyxml

systemd = dependency('systemd')
zlib = dependency('zlib')
bmcweb_dependencies += [systemd, zlib]

nlohmann_json_dep = dependency('nlohmann_json', version: '>=3.11.2', include_type: 'system')
bmcweb_dependencies += nlohmann_json_dep

boost = dependency(
  'boost',
  modules: [
    'url',
    ],
  version : '>=1.84.0',
  required : false,
  include_type: 'system'
)
if boost.found()
  bmcweb_dependencies += [boost]
else
  cmake = import('cmake')
  opt = cmake.subproject_options()
  opt.add_cmake_defines({
    'BOOST_INCLUDE_LIBRARIES': 'asio;beast;callable_traits;headers;process;type_index;url;uuid'
  })
  boost = cmake.subproject('boost', required: true, options: opt)
  boost_asio = boost.dependency('boost_asio').as_system()
  boost_beast = boost.dependency('boost_beast').as_system()
  boost_callable_traits = boost.dependency('boost_callable_traits').as_system()
  boost_headers = boost.dependency('boost_headers').as_system()
  boost_process = boost.dependency('boost_process').as_system()
  boost_type_index = boost.dependency('boost_type_index').as_system()
  boost_url = boost.dependency('boost_url').as_system()
  boost_uuid = boost.dependency('boost_uuid').as_system()
  bmcweb_dependencies += [
    boost_asio,
    boost_beast,
    boost_callable_traits,
    boost_headers,
    boost_process,
    boost_type_index,
    boost_url,
    boost_uuid
  ]
endif

if get_option('tests').allowed()
  gtest = dependency('gtest', main: true, disabler: true, required : false)
  gmock = dependency('gmock', required : false)
  if not gtest.found() and get_option('tests').allowed()
    gtest_proj = subproject('gtest', required: true)
    gtest = gtest_proj.get_variable('gtest_main_dep')
    gmock = gtest_proj.get_variable('gmock_dep')
  endif
  gtest = gtest.as_system('system')
  gmock = gmock.as_system('system')
endif

systemd_system_unit_dir = systemd.get_variable('systemdsystemunitdir')

bindir = get_option('prefix') + '/' +get_option('bindir')

summary({
          'prefix' : get_option('prefix'),
          'bindir' : bindir,
          'systemd unit directory' : systemd_system_unit_dir
        }, section : 'Directories')

install_subdir('static', install_dir : 'share/www', strip_directory : true)

# Config subdirectory

subdir('config')
bmcweb_dependencies += conf_h_dep

# Source files
fs = import('fs')

srcfiles_bmcweb = files(
  'redfish-core/src/error_messages.cpp',
  'redfish-core/src/registries.cpp',
  'redfish-core/src/utils/time_utils.cpp',
  'redfish-core/src/utils/json_utils.cpp',
  'src/boost_asio_ssl.cpp',
  'src/boost_asio.cpp',
  'src/boost_beast.cpp',
  'src/dbus_singleton.cpp',
  'src/json_html_serializer.cpp',
  'src/ossl_random.cpp',
)

bmcweblib = static_library(
  'bmcweblib',
  srcfiles_bmcweb,
  include_directories : incdir,
  dependencies: bmcweb_dependencies,
)

# Generate the bmcweb executable
executable(
  'bmcweb',
  'src/webserver_main.cpp',
  include_directories : incdir,
  dependencies: bmcweb_dependencies,
  link_with: bmcweblib,
  link_args: '-Wl,--gc-sections',
  install: true,
  install_dir:bindir
)

srcfiles_unittest = files(
  'test/http/crow_getroutes_test.cpp',
  'test/http/http_connection_test.cpp',
  'test/http/router_test.cpp',
  'test/http/http_response_test.cpp',
  'test/http/utility_test.cpp',
  'test/http/verb_test.cpp',
  'test/http/mutual_tls_meta.cpp',
  'test/include/dbus_utility_test.cpp',
  'test/include/google/google_service_root_test.cpp',
  'test/include/json_html_serializer.cpp',
  'test/include/http_utility_test.cpp',
  'test/include/human_sort_test.cpp',
  'test/include/async_resolve_test.cpp',
  'test/include/credential_pipe_test.cpp',
  'test/include/ibm/configfile_test.cpp',
  'test/include/ibm/lock_test.cpp',
  'test/include/multipart_test.cpp',
  'test/include/openbmc_dbus_rest_test.cpp',
  'test/include/str_utility_test.cpp',
  'test/include/ossl_random.cpp',
  'test/redfish-core/include/privileges_test.cpp',
  'test/redfish-core/include/redfish_aggregator_test.cpp',
  'test/redfish-core/include/registries_test.cpp',
  'test/redfish-core/include/utils/hex_utils_test.cpp',
  'test/redfish-core/include/utils/ip_utils_test.cpp',
  'test/redfish-core/include/utils/json_utils_test.cpp',
  'test/redfish-core/include/utils/query_param_test.cpp',
  'test/redfish-core/include/utils/stl_utils_test.cpp',
  'test/redfish-core/include/utils/time_utils_test.cpp',
  'test/redfish-core/lib/chassis_test.cpp',
  'test/redfish-core/lib/sensors_test.cpp',
  'test/redfish-core/lib/log_services_dump_test.cpp',
  'test/redfish-core/lib/log_services_test.cpp',
  'test/redfish-core/lib/update_service_test.cpp',
  'test/redfish-core/lib/service_root_test.cpp',
  'test/redfish-core/lib/thermal_subsystem_test.cpp',
  'test/redfish-core/lib/power_subsystem_test.cpp',
)

if (get_option('audit-events').allowed())
	srcfiles_unittest += files('test/include/audit_events_test.cpp',)
endif

if(get_option('tests').allowed())
    # generate the test executable
    foreach test_src : srcfiles_unittest
      test_bin = executable(
        fs.stem(test_src),
        test_src,
        link_with: bmcweblib,
        include_directories : incdir,
        install_dir: bindir,
        dependencies: bmcweb_dependencies + [
          gtest,
          gmock,
        ]
      )
      test(fs.stem(test_src), test_bin)
    endforeach
endif
