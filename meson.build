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
  'redfish-post-to-old-updateservice'           : '-DBMCWEB_ENABLE_REDFISH_UPDATESERVICE_OLD_POST_URL',
  'redfish'                                     : '-DBMCWEB_ENABLE_REDFISH',
  'rest'                                        : '-DBMCWEB_ENABLE_DBUS_REST',
  'session-auth'                                : '-DBMCWEB_ENABLE_SESSION_AUTHENTICATION',
  'static-hosting'                              : '-DBMCWEB_ENABLE_STATIC_HOSTING',
  'vm-websocket'                                : '-DBMCWEB_ENABLE_VM_WEBSOCKET',
  'xtoken-auth'                                 : '-DBMCWEB_ENABLE_XTOKEN_AUTHENTICATION',
  #'vm-nbdproxy'                                : '-DBMCWEB_ENABLE_VM_NBDPROXY',
}

# Get the options status and build a project summary to show which flags are
# being enabled during the configuration time.

foreach option_key,option_value : feature_map
  if(get_option(option_key).enabled())
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

if(get_option('tests').enabled())
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

if( get_option('bmcweb-logging').enabled() or \
    get_option('buildtype').startswith('debug'))
 add_project_arguments([
   '-DBMCWEB_ENABLE_LOGGING',
   '-DBMCWEB_ENABLE_DEBUG'
   ],
  language : 'cpp')

  summary({'debug' :'-DBMCWEB_ENABLE_DEBUG',
     'logging' : '-DBMCWEB_ENABLE_LOGGING',

    },section : 'Enabled Features')
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
  '-DBOOST_ALL_NO_LIB',
  '-DBOOST_ALLOW_DEPRECATED_HEADERS',
  '-DBOOST_ASIO_DISABLE_THREADS',
  '-DBOOST_ASIO_NO_DEPRECATED',
  '-DBOOST_ASIO_SEPARATE_COMPILATION',
  '-DBOOST_BEAST_SEPARATE_COMPILATION',
  '-DBOOST_EXCEPTION_DISABLE',
  '-DBOOST_URL_NO_SOURCE_LOCATION',
  '-DJSON_NOEXCEPTION',
  '-DOPENSSL_NO_FILENAMES',
]),
language : 'cpp')

# Find the dependency modules, if not found use meson wrap to get them
# automatically during the configure step
bmcweb_dependencies = []

pam = cxx.find_library('pam', required: true)
atomic =  cxx.find_library('atomic', required: true)
openssl = dependency('openssl', required : true)
bmcweb_dependencies += [pam, atomic, openssl]

sdbusplus = dependency('sdbusplus', required : false, include_type: 'system')
if not sdbusplus.found()
  sdbusplus_proj = subproject('sdbusplus', required: true)
  sdbusplus = sdbusplus_proj.get_variable('sdbusplus_dep')
  sdbusplus = sdbusplus.as_system('system')
endif
bmcweb_dependencies += sdbusplus

tinyxml = dependency('tinyxml2',
    default_options: ['tests=false'],
    include_type: 'system',
)
bmcweb_dependencies += tinyxml

systemd = dependency('systemd')
zlib = dependency('zlib')
bmcweb_dependencies += [systemd, zlib]

if cxx.has_header('nlohmann/json.hpp')
    nlohmann_json = declare_dependency()
else
    nlohmann_json_proj = subproject('nlohmann', required: true)
    nlohmann_json = nlohmann_json_proj.get_variable('nlohmann_json_dep')
    nlohmann_json = nlohmann_json.as_system('system')
endif
bmcweb_dependencies += nlohmann_json

boost = dependency('boost',version : '>=1.81.0', required : false, include_type: 'system')
if not boost.found()
  boost = subproject('boost', required: true).get_variable('boost_dep')
  boost = boost.as_system('system')
endif
bmcweb_dependencies += boost

if get_option('tests').enabled()
  gtest = dependency('gtest', main: true,disabler: true, required : false)
  gmock = dependency('gmock', required : false)
  if not gtest.found() and get_option('tests').enabled()
    gtest_proj = subproject('gtest', required: true)
    gtest = gtest_proj.get_variable('gtest_main_dep')
    gmock = gtest_proj.get_variable('gmock_dep')
  endif
  gtest = gtest.as_system('system')
  gmock = gmock.as_system('system')
endif

systemd_system_unit_dir = systemd.get_variable(pkgconfig: 'systemdsystemunitdir')

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
  'redfish-core/src/utils/json_utils.cpp',
  'src/boost_asio_ssl.cpp',
  'src/boost_asio.cpp',
  'src/boost_beast.cpp',
  'src/boost_url.cpp',
  'src/dbus_singleton.cpp',
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
  'test/http/router_test.cpp',
  'test/http/utility_test.cpp',
  'test/http/verb_test.cpp',
  'test/include/dbus_utility_test.cpp',
  'test/include/google/google_service_root_test.cpp',
  'test/include/http_utility_test.cpp',
  'test/include/human_sort_test.cpp',
  'test/include/ibm/configfile_test.cpp',
  'test/include/ibm/lock_test.cpp',
  'test/include/multipart_test.cpp',
  'test/include/openbmc_dbus_rest_test.cpp',
  'test/include/str_utility_test.cpp',
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
  'test/redfish-core/lib/service_root_test.cpp',
  'test/redfish-core/lib/thermal_subsystem_test.cpp',
  'test/redfish-core/lib/power_subsystem_test.cpp',
  'test/src/security_headers_test.cpp',
)

if(get_option('tests').enabled())
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
