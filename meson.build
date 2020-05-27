project('bmcweb', 'cpp',
         version : '1.0',
         meson_version: '>=0.53.2',
         default_options: [
         'warning_level=3',
         'cpp_std=c++17',
        ])
####################################################################
# Project related links

project_pretty_name = 'bmcweb'
project_url = 'https://github.com/openbmc/' + project_pretty_name
project_issues_url_ = project_url + '/issues/new'

#####################################################################
# Validate the c++ Standard

if get_option('cpp_std') != 'c++17'
    error('This project requires cpp std to be in c++17 mode')
endif

#####################################################################
# Get compiler and default build type

cxx = meson.get_compiler('cpp')
build = get_option('buildtype')
summary('Build Type',build, section : 'Build Info')

#########################################################################
# Find the dependency modules

cppfs	  =	cxx.find_library('stdc++fs')
pam	  =	cxx.find_library('pam', required: true)
atomic	  =	cxx.find_library('atomic', required: true)
sdbusplus =	dependency('sdbusplus',required : false)
openssl	  =	dependency('openssl')
tinyxml	  =	dependency('tinyxml2', required: false)
systemd	  =	dependency('systemd')
zlib	  =	dependency('zlib')
boost	  =	dependency('boost',version : '1.71', required : false)
gtest	  =	dependency('gtest', main: true, disabler: true,required : get_option('unittest'))
gmock	  =	dependency('gmock',required : get_option('unittest'))

###############################################################################
# Include Directories

incdir = include_directories('include','redfish-core/include',
                             'redfish-core/lib','http')

###############################################################################
# Get the options and enable the respective features

# create a MAP of  "options : feature_flag"

feature_map = {
'pam'                             : '-DWEBSERVER_ENABLE_PAM',
'kvm'                             : '-DBMCWEB_ENABLE_KVM' ,
'vm-websocket'                    : '-DBMCWEB_ENABLE_VM_WEBSOCKET',
'vm-nbdproxy'                     : '-DBMCWEB_ENABLE_VM_NBDPROXY',
'rest'                            : '-DBMCWEB_ENABLE_DBUS_REST',
'redfish'                         : '-DBMCWEB_ENABLE_REDFISH',
'host-serial-socket'              : '-DBMCWEB_ENABLE_HOST_SERIAL_WEBSOCKET',
'static-hosting'                  : '-DBMCWEB_ENABLE_STATIC_HOSTING',
'redfish-bmc-journal'             : '-DBMCWEB_ENABLE_REDFISH_BMC_JOURNAL',
'redfish-raw-peci'                : '-DBMCWEB_ENABLE_REDFISH_RAW_PECI',
'redfish-cpu-log'                 : '-DBMCWEB_ENABLE_REDFISH_CPU_LOG',
'redfish-sysdump-log'             : '-DBMCWEB_ENABLE_REDFISH_SYSTEMDUMP_LOG',
'redfish-dbus-log'                : '-DBMCWEB_ENABLE_REDFISH_DBUS_LOG_ENTRIES',
'redfish-provisioning-feature'    : '-DBMCWEB_ENABLE_REDFISH_PROVISIONING_FEATURE',
'mutual-tls-auth'                 : '-DBMCWEB_ENABLE_MUTUAL_TLS_AUTHENTICATION',
'ibm-management-console'          : '-DBMCWEB_ENABLE_IBM_MANAGEMENT_CONSOLE',
'disable-csrf'                    : '-DBMCWEB_INSECURE_DISABLE_CSRF_PREVENTION',
'disable-ssl'                     : '-DBMCWEB_INSECURE_DISABLE_SSL',
'disable-auth'                    : '-DBMCWEB_INSECURE_DISABLE_AUTHENTICATION',
'disable-xss'                     : '-DBMCWEB_INSECURE_DISABLE_XSS_PREVENTION',
'tftp-update'                     : '-DBMCWEB_INSECURE_ENABLE_REDFISH_FW_TFTP_UPDATE',
'push-style-notification'         : '-DBMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING',
'validate-unsecure-feature'       : '-DBMCWEB_ENABLE_VALIDATION_UNSECURE_FEATURE',
'sensor-override'                 : '-DBMCWEB_INSECURE_UNRESTRICTED_SENSOR_OVERRIDE',
}

# Get the options status and build a project summary to show which flags are
# being enabled during the configuration time.

foreach option_key,option_value : feature_map
  if(get_option(option_key).enabled())
    if(option_key == 'mutual-tls-auth' or option_key == 'disable-ssl')
      if(get_option('disable-ssl').disabled() or get_option('mutual-tls-auth').disabled())
        add_global_arguments(option_value,language:'cpp')
        summary(option_key,option_value, section : 'Enabled Features')
      endif
    else
      summary(option_key,option_value, section : 'Enabled Features')
      add_global_arguments(option_value,language:'cpp')
    endif
  else
      if(option_key == 'disable-ssl')
        summary('ssl','-DBMCWEB_ENABLE_SSL', section : 'Enabled Features')
        add_global_arguments('-DBMCWEB_ENABLE_SSL', language : 'cpp')
      endif
  endif
endforeach

if(get_option('build-static-libs').enabled())
  summary('build-static-libs','NA', section : 'Enabled Features')
endif

if(get_option('unittest').enabled())
  summary('unittest','NA', section : 'Enabled Features')
endif

################################################################################
# Add compiler arguments

# Wno-psabi reduces the number of "Note:" messages when cross-compiling some STL
# stuff for ARM.
# See https://stackoverflow.com/questions/48149323/strange-gcc-warning-when-compiling-qt-project
# Basically, gcc 6 and gcc 7 are not ABI compatible, but since the whole OpenBMC
# project uses the same compiler, we can safely ignmore these info notes.

add_global_arguments('-Wno-psabi', language: 'cpp')

add_global_arguments(
  cxx.get_supported_arguments([
  '-Wnon-virtual-dtor',
  '-Wold-style-cast','-Wcast-align',
  '-Wunused','-Woverloaded-virtual',
  '-Wconversion','-Wsign-conversion','-Wnull-dereference',
  '-Wdouble-promotion','-Wformat=2',
  '-Wno-unused-parameter','-fno-rtti',
  '-Wno-attributes'
  ]),
  language: 'cpp'
)

# if compiler is gnu-gcc , and version is > 8.0 then we have to add Werror
# and few more compiler arguments , we know that will pass

if (cxx.get_id() == 'gcc' and cxx.version().version_compare('>8.0'))
  add_global_arguments(
  cxx.get_supported_arguments([
  '-Wduplicated-cond',
  '-Wduplicated-branches',
  '-Wlogical-op',
  '-Werror'
  ]),
  language: 'cpp')

  if get_option('buildtype').startswith('debug')
    # Reduce the binary size by removing unnecessary
    # dynamic symbol table entries

    add_global_arguments(
    cxx.get_supported_arguments([
    '-fno-fat-lto-objects',
    '-fvisibility=hidden',
    '-fvisibility-inlines-hidden'
    ]),
    language: 'cpp')

    summary('debug','-DBMCWEB_ENABLE_DEBUG', section : 'Enabled Features')
    summary('logging','-DBMCWEB_ENABLE_LOGGING', section : 'Enabled Features')
    add_global_arguments([
    '-DBMCWEB_ENABLE_LOGGING',
    '-DBMCWEB_ENABLE_DEBUG'
    ],
    language : 'cpp')

    if cxx.has_link_argument('-Wl,--exclude-libs,ALL')
      add_global_link_arguments('-Wl,--exclude-libs,ALL', language: 'cpp')
    endif

  endif
endif

######################################################################
# Set Compiler Security flags

security_flags = [
'-fstack-protector-strong',
'-fPIE',
'-fPIC',
'-D_FORTIFY_SOURCE=2',
'-Wformat',
'-Wformat-security'
]

## Add security flags for builds of type 'release' and 'debugoptimized'

if (get_option('buildtype') == 'release' or get_option('buildtype') == 'debugoptimized')
  add_global_arguments(
  cxx.get_supported_arguments([
  security_flags
  ]),
  language: 'cpp')

  # 'release' and 'debugoptimized' buildtypes comes by default with -O2
  #  optimization , so we are good to add '-flto'
  add_global_arguments(
  cxx.get_supported_arguments([
  '-flto'
  ]),
  language: 'cpp')
endif

###########################################################################
# Boost dependency configuration

add_global_arguments(
cxx.get_supported_arguments([
'-DBOOST_ASIO_DISABLE_THREADS',
'-DBOOST_BEAST_USE_STD_STRING_VIEW',
'-DBOOST_ERROR_CODE_HEADER_ONLY',
'-DBOOST_SYSTEM_NO_DEPRECATED',
'-DBOOST_ALL_NO_LIB',
'-DBOOST_NO_RTTI',
'-DBOOST_NO_TYPEID',
'-DBOOST_COROUTINES_NO_DEPRECATION_WARNING'
]),
language : 'cpp')

###############################################################################
# Source files

srcfiles_bmcweb = ['redfish-core/src/error_messages.cpp',
                   'redfish-core/src/utils/json_utils.cpp']

srcfiles_unittest = ['redfish-core/ut/privileges_test.cpp',
		     'src/msan_test.cpp']

###############################################################################
# Gather the Configuration data

conf_data = configuration_data()
conf_data.set('BMCWEB_HTTP_REQ_BODY_LIMIT_MB',30)
configure_file(output: 'config.h',
	       configuration: conf_data)

################################################################################
# Build the libbmcweb Static Library if the Corresponding Flag is On

if(get_option('build-static-libs').enabled() or get_option('unittest').enabled())
  libbmcweb = static_library('libbmcweb',
			    srcfiles_bmcweb,
			    include_directories : incdir,
			    dependencies:[cppfs,pam,atomic,sdbusplus,tinyxml,systemd,zlib,boost])
endif

################################################################################
# Configure and install systemd unit files

systemd_system_unit_dir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

prefix = get_option('prefix')
systemd_system_unit_dir = prefix + systemd_system_unit_dir
bindir = prefix + '/' +get_option('bindir')

summary('prefix',prefix , section : 'Directories')
summary('bindir',bindir , section : 'Directories')
summary('systemd unit directory',systemd_system_unit_dir, section : 'Directories')

configure_file(input : 'bmcweb.socket',
               output : 'bmcweb.socket',
               copy : true,
               install_dir: systemd_system_unit_dir,
               install : true)

configure_file(input : 'bmcweb.service.in',
               output : 'bmcweb.service',
               install_dir: systemd_system_unit_dir,
               copy : true,
	       install : true)
################################################################################
# Generate the bmcweb executable and the test binary

srcfiles_bmcweb += 'src/webserver_main.cpp'
executable('bmcweb',
	    srcfiles_bmcweb,
	    include_directories : incdir,
	    dependencies: [ 
			      cppfs,
			      pam,
			      atomic,
			      sdbusplus,
			      openssl,
			      tinyxml,
			      systemd,
			      zlib,
			      boost
			  ],
	    install: true,
	    install_dir:bindir)

if(get_option('unittest').enabled())
  foreach src_test : srcfiles_unittest
    testname = src_test.split('/')[-1]
    test(testname,executable(testname,src_test,
                include_directories : incdir,
                link_with: [libbmcweb],
                install_dir: bindir,
                dependencies: [
				gtest,
				gmock
			      ]))
  endforeach
endif
