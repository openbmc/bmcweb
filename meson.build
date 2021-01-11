project('bmcweb', 'cpp',
        version : '1.0',
        meson_version: '>=0.53.2',
        default_options: [
            'werror=true',
            'warning_level=3',
            'cpp_std=c++17',
            'buildtype=debugoptimized',
            'b_ndebug=if-release',
            'b_lto=true',
# enable when https://github.com/chriskohlhoff/asio/issues/533
# is resolved. ASIO default executor doesn't build with no-rtti
           #'cpp_rtti=false'
           ])

# Project related links

project_pretty_name = 'bmcweb'
project_url = 'https://github.com/openbmc/' + project_pretty_name
project_issues_url = project_url + '/issues/new'
summary('Issues',project_issues_url, section: 'Report Issues')

# Validate the c++ Standard

if get_option('cpp_std') != 'c++17'
    error('This project requires c++17 support')
endif

# Get compiler and default build type

cxx = meson.get_compiler('cpp')
build = get_option('buildtype')
optimization = get_option('optimization')
summary('Build Type',build, section : 'Build Info')
summary('Optimization',optimization, section : 'Build Info')

# remove debug information for minsize buildtype
if(get_option('buildtype') == 'minsize')
  add_project_arguments('-DNDEBUG', language : 'cpp')
endif

# Disable lto when compiling with no optimization
if(get_option('optimization') == '0')
  add_project_arguments('-fno-lto', language: 'cpp')
  message('Disabling lto & its supported features as optimization is disabled')
endif

# Include Directories

incdir = include_directories('include','redfish-core/include',
                             'redfish-core/lib','http')

# Get the options and enable the respective features
## create a MAP of  "options : feature_flag"

feature_map = {
'insecure-disable-auth'           : '-DBMCWEB_INSECURE_DISABLE_AUTHENTICATION',
'insecure-disable-csrf'           : '-DBMCWEB_INSECURE_DISABLE_CSRF_PREVENTION',
'insecure-disable-ssl'            : '-DBMCWEB_INSECURE_DISABLE_SSL',
'insecure-disable-xss'            : '-DBMCWEB_INSECURE_DISABLE_XSS_PREVENTION',
'host-serial-socket'              : '-DBMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET',
'ibm-management-console'          : '-DBMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE',
'kvm'                             : '-DBMCWEB_ENABLE_KVM' ,
'basic-auth'                      : '-DBMCWEB_ENABLE_BASIC_AUTHENTICATION',
'session-auth'                    : '-DBMCWEB_ENABLE_SESSION_AUTHENTICATION',
'xtoken-auth'                     : '-DBMCWEB_ENABLE_XTOKEN_AUTHENTICATION',
'cookie-auth'                     : '-DBMCWEB_ENABLE_COOKIE_AUTHENTICATION',
'mutual-tls-auth'                 : '-DBMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION',
'pam'                             : '-DWEBSERVER_ENABLE_PAM',
'insecure-push-style-notification': '-DBMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING',
'redfish'                         : '-DBMCWEB_ENABLE_REDFISH',
'redfish-bmc-journal'             : '-DBMCWEB_ENABLE_REDFISH_BMC_JOURNAL',
'redfish-cpu-log'                 : '-DBMCWEB_ENABLE_REDFISH_CPU_LOG',
'redfish-dbus-log'                : '-DBMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES',
'redfish-provisioning-feature'    : '-DBMCWEB_ENABLE_REDFISH_PROVISIONING_FEATURE',
'redfish-raw-peci'                : '-DBMCWEB_ENABLE_REDFISH_RAW_PECI',
'redfish-dump-log'                : '-DBMCWEB_ENABLE_REDFISH_DUMP_LOG',
'rest'                            : '-DBMCWEB_ENABLE_DBUS_REST',
'insecure-sensor-override'        : '-DBMCWEB_INSECURE_UNRESTRICTED_SENSOR_OVERRIDE',
'static-hosting'                  : '-DBMCWEB_ENABLE_STATIC_HOSTING',
'insecure-tftp-update'            : '-DBMCWEB_INSECURE_ENABLE_REDFISH_FW_TFTP_UPDATE',
'validate-unsecure-feature'       : '-DBMCWEB_ENABLE_VALIDATION_UNSECURE_FEATURE',
'vm-nbdproxy'                     : '-DBMCWEB_ENABLE_VM_NBDPROXY',
'vm-websocket'                    : '-DBMCWEB_ENABLE_VM_WEBSOCKET',
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

if get_option('yocto-deps').disabled()
  # This is an out of tree build, so enable ibm console flag
  # for CI purpose.
  add_project_arguments('-DBMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE', language: 'cpp')
  summary('ibm-management-console','-DBMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE',section : 'Enabled Features')
endif

# Add compiler arguments

# -Wpedantic, -Wextra comes by default with warning level
add_project_arguments(
  cxx.get_supported_arguments([
  '-Wold-style-cast',
  '-Wcast-align',
  '-Wunused',
  '-Woverloaded-virtual',
  '-Wconversion',
  '-Wsign-conversion',
  '-Wno-attributes',
  ]),
  language: 'cpp'
)

if (cxx.get_id() == 'clang' and cxx.version().version_compare('>9.0'))
add_project_arguments(
  cxx.get_supported_arguments([
    '-Weverything',
    '-Wno-c++98-compat',
    '-Wno-c++98-compat-pedantic',
    '-Wno-global-constructors',
    '-Wno-exit-time-destructors',
    '-Wno-shadow',
    '-Wno-used-but-marked-unused',
    '-Wno-documentation-unknown-command',
    '-Wno-weak-vtables',
    '-Wno-documentation',
    '-Wno-padded',
    '-Wunused-parameter',
    '-Wcovered-switch-default',
    '-Wcomma',
    '-Wextra-semi',
    '-Wzero-as-null-pointer-constant',
    '-Wswitch-enum',
    '-Wnull-dereference',
    '-Wdouble-promotion',
    '-Wformat=2',
  ]),
  language:'cpp')
endif

# if compiler is gnu-gcc , and version is > 8.0 then we add few more
# compiler arguments , we know that will pass

if (cxx.get_id() == 'gcc' and cxx.version().version_compare('>8.0'))

  ## remove once bmcweb/issues/147 is fixed
  add_global_link_arguments('-Wno-stringop-overflow',language:['c','cpp'])
  add_project_arguments('-Wno-stringop-overflow',language:['c','cpp'])

  add_project_arguments(
    cxx.get_supported_arguments([
     '-Wduplicated-cond',
     '-Wduplicated-branches',
     '-Wlogical-op',
     '-Wunused-parameter',
     '-Wnull-dereference',
     '-Wdouble-promotion',
     '-Wformat=2',
     ]),
    language:'cpp')

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

  endif
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
'-DBOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT',
'-DBOOST_ASIO_DISABLE_THREADS',
'-DBOOST_BEAST_USE_STD_STRING_VIEW',
'-DBOOST_ERROR_CODE_HEADER_ONLY',
'-DBOOST_SYSTEM_NO_DEPRECATED',
'-DBOOST_ASIO_NO_DEPRECATED',
'-DBOOST_ALL_NO_LIB',
'-DBOOST_NO_RTTI',
'-DBOOST_NO_TYPEID',
'-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
'-DBOOST_URL_STANDALONE',
'-DBOOST_URL_HEADER_ONLY',
'-DBOOST_ALLOW_DEPRECATED_HEADERS'
]),
language : 'cpp')

# Find the dependency modules, if not found use meson wrap to get them
# automatically during the configure step
bmcweb_dependencies = []

pam = cxx.find_library('pam', required: get_option('pam'))
atomic =  cxx.find_library('atomic', required: true)
openssl = dependency('openssl', required : true)
bmcweb_dependencies += [pam, atomic, openssl]

sdbusplus = dependency('sdbusplus',required : false)
if not sdbusplus.found()
  sdbusplus_proj = subproject('sdbusplus', required: true)
  sdbusplus = sdbusplus_proj.get_variable('sdbusplus_dep')
  sdbusplus = sdbusplus.as_system('system')
endif
bmcweb_dependencies += sdbusplus

if get_option('rest').enabled()
  tinyxml = dependency('tinyxml2', required: false)
  if not tinyxml.found()
    tinyxml_proj = subproject('tinyxml2', required: true)
    tinyxml = tinyxml_proj.get_variable('tinyxml2_dep')
    tinyxml = tinyxml.as_system('system')
  endif
  bmcweb_dependencies += tinyxml
endif

systemd = dependency('systemd')
zlib = dependency('zlib')
bmcweb_dependencies += [systemd, zlib]

if cxx.has_header('nlohmann/json.hpp')
    nlohmann_json = declare_dependency()
else
    subproject('nlohmann', required: false)
    nlohmann_json = declare_dependency(
        include_directories: [
            'subprojects/nlohmann/single_include',
            'subprojects/nlohmann/single_include/nlohmann',
        ]
    )
endif
bmcweb_dependencies += nlohmann_json

boost = dependency('boost',version : '>=1.73.0', required : false)
if not boost.found()
  subproject('boost', required: false)
  boost_inc = include_directories('subprojects/boost_1_73_0/', is_system:true)
  boost  = declare_dependency(include_directories : boost_inc)
endif
bmcweb_dependencies += boost

if cxx.has_header('boost/url/url_view.hpp')
  boost_url = declare_dependency()
else
  subproject('boost-url', required: false)
  boost_url_inc = include_directories('subprojects/boost-url/include', is_system:true)
  boost_url= declare_dependency(include_directories : boost_url_inc)
endif
bmcweb_dependencies += boost_url

if get_option('tests').enabled()
  gtest = dependency('gtest', main: true,disabler: true, required : false)
  gmock = dependency('gmock', required : false)
  if not gtest.found() and get_option('tests').enabled()
    gtest_proj = subproject('gtest', required: true)
    gtest = gtest_proj.get_variable('gtest_main_dep')
    gmock = gtest_proj.get_variable('gmock_dep')
  endif
endif

# Source files

srcfiles_bmcweb = ['src/webserver_main.cpp','redfish-core/src/error_messages.cpp',
                   'redfish-core/src/utils/json_utils.cpp']

srcfiles_unittest = ['include/ut/dbus_utility_test.cpp',
                     'redfish-core/ut/privileges_test.cpp',
                     'redfish-core/ut/lock_test.cpp',
                     'http/ut/utility_test.cpp']

# Gather the Configuration data

conf_data = configuration_data()
conf_data.set('BMCWEB_HTTP_REQ_BODY_LIMIT_MB',get_option('http-body-limit'))
conf_data.set('MESON_INSTALL_PREFIX',get_option('prefix'))
configure_file(output: 'config.h',
               configuration: conf_data)

# Configure and install systemd unit files

systemd_system_unit_dir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

bindir = get_option('prefix') + '/' +get_option('bindir')

summary({
          'prefix' : get_option('prefix'),
          'bindir' : bindir,
          'systemd unit directory' : systemd_system_unit_dir
        }, section : 'Directories')

configure_file(input : 'bmcweb.socket',
               output : 'bmcweb.socket',
               copy : true,
               install_dir: systemd_system_unit_dir,
               install : true)

configure_file(input : 'bmcweb.service.in',
               output : 'bmcweb.service',
               install_dir: systemd_system_unit_dir,
               configuration: conf_data,
               install : true)

# Copy pam-webserver to etc/pam.d
configure_file(input : 'pam-webserver',
               output : 'webserver',
               copy : true,
               install_dir: '/etc/pam.d',
               install : true)

install_subdir('static', install_dir : 'share/www', strip_directory : true)

# Generate the bmcweb executable and the test binary

executable('bmcweb',srcfiles_bmcweb,
            include_directories : incdir,
            dependencies: bmcweb_dependencies,
            install: true,
            install_dir:bindir)

if(get_option('tests').enabled())
  foreach src_test : srcfiles_unittest
    testname = src_test.split('/')[-1].split('.')[0]
    test(testname,executable(testname,src_test,
                include_directories : incdir,
                install_dir: bindir,
                dependencies: [
                                boost, boost_url, gtest,openssl,gmock,nlohmann_json,sdbusplus
                              ]))
  endforeach
endif
