project('bmcweb', 'cpp',
        default_options: [
          'cpp_std=c++17',
          'b_lundef=false',
          'b_lto=true',
        ])
cxx = meson.get_compiler('cpp')



BOOST_ROOT = 'builddir/boost'

boost_dep = dependency('boost', modules: [])
boost_defines = [
'-DBOOST_ASIO_DISABLE_THREADS',
'-DBOOST_ERROR_CODE_HEADER_ONLY',
'-DBOOST_SYSTEM_NO_DEPRECATED',
'-DBOOST_ALL_NO_LIB',
'-DBOOST_NO_RTTI',
'-DBOOST_NO_TYPEID',
'-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
'-DBMCWEB_ENABLE_SSL',
]
add_project_arguments(boost_defines, language: 'cpp')

security_flags = [
'-fstack-protector-strong',
'-fPIE',
'-fPIC',
'-D_FORTIFY_SOURCE=2',
'-Wformat',
'-Wformat-security',
]


add_project_arguments(security_flags, language : 'cpp')

log_flags = [
'-DBMCWEB_ENABLE_LOGGING',
'-DBMCWEB_ENABLE_DEBUG',
]

add_project_arguments(log_flags, language : 'cpp')


cc = meson.get_compiler('cpp')
pam = cc.find_library('pam', required: true)
sdbusplus = dependency('sdbusplus')
openssl = dependency('openssl')

incdir = include_directories('include', 'redfish-core/include', 'crow/include')
executable('bmcweb', 'src/webserver_main.cpp', 
include_directories : incdir, 
dependencies: [boost_dep, sdbusplus, openssl, pam],
install: true)
