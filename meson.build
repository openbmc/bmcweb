project(
    'bmcweb',
    'cpp',
    version: '1.0',
    meson_version: '>=1.3.0',
    default_options: [
        'b_lto_mode=default',
        'b_lto_threads=0',
        'b_lto=true',
        'b_ndebug=if-release',
        'buildtype=debugoptimized',
        'cpp_rtti=false',
        'cpp_std=c++23',
        'warning_level=3',
        'werror=true',
    ],
)

# Project related links

project_pretty_name = 'bmcweb'
project_url = 'https://github.com/openbmc/' + project_pretty_name
project_issues_url = project_url + '/issues/new'
summary('Issues', project_issues_url, section: 'Report Issues')

# Validate the c++ Standard

if get_option('cpp_std') != 'c++23'
    error('This project requires c++23 support')
endif

# Get compiler and default build type

cxx = meson.get_compiler('cpp')
build = get_option('buildtype')
optimization = get_option('optimization')
summary('Build Type', build, section: 'Build Info')
summary('Optimization', optimization, section: 'Build Info')

# remove debug information for minsize buildtype
if (get_option('buildtype') == 'minsize')
    add_project_arguments(
        ['-fdata-sections', '-ffunction-sections'],
        language: 'cpp',
    )
    add_project_arguments('-DNDEBUG', language: 'cpp')
endif

if (get_option('dns-resolver') == 'systemd-dbus')
    add_project_arguments('-DBMCWEB_DBUS_DNS_RESOLVER', language: 'cpp')
endif

# Disable lto when compiling with no optimization
if (get_option('optimization') == '0')
    add_project_arguments('-fno-lto', language: 'cpp')
    message('Disabling lto & its supported features as optimization is disabled')
endif

# Include Directories
incdir = [
    include_directories(
        'include',
        'redfish-core/include',
        'redfish-core/lib',
        'http',
    ),
]

# Add compiler arguments
boost_flags = ['-Wno-pedantic']
nghttp2_flags = []
if (cxx.get_id() == 'clang')
    if (cxx.version().version_compare('<17.0'))
        error('This project requires clang-17 or higher')
    endif
    add_project_arguments(
        '-Weverything',
        '-Wformat=2',
        # https://github.com/llvm/llvm-project/issues/101614
        '-fno-builtin-std-forward_like',
        '-Wno-c++20-extensions',
        '-Wno-c++26-extensions',
        '-Wno-c++98-compat',
        '-Wno-c++98-compat-pedantic',
        '-Wno-covered-switch-default',
        '-Wno-disabled-macro-expansion',
        '-Wno-documentation',
        '-Wno-documentation-unknown-command',
        '-Wno-exit-time-destructors',
        '-Wno-global-constructors',
        '-Wno-missing-include-dirs',
        '-Wno-newline-eof',
        '-Wno-padded',
        '-Wno-shadow',
        '-Wno-switch-enum',
        '-Wno-unneeded-internal-declaration',
        '-Wno-unsafe-buffer-usage-in-container',
        '-Wno-unsafe-buffer-usage-in-libc-call',
        '-Wno-unused-macros',
        '-Wno-used-but-marked-unused',
        '-Wno-weak-vtables',
        language: 'cpp',
    )
    if (cxx.version().version_compare('>=21.0'))
        # TODO(Ed) These warnings look valid and need cleaned up.
        add_project_arguments(
            '-Wno-unique-object-duplication',
            '-Wno-nrvo',
            language: 'cpp',
        )
    endif

    boost_flags += ['-Wno-strict-prototypes', '-Wno-unused-but-set-variable']
    nghttp2_flags += ['-Wno-extra-semi']
endif

if (cxx.get_id() == 'gcc')
    if (cxx.version().version_compare('<13.0'))
        error('This project requires gcc-13 or higher')
    endif

    add_project_arguments(
        '-Wformat=2',
        '-Wcast-align',
        '-Wconversion',
        '-Woverloaded-virtual',
        '-Wsign-conversion',
        '-Wunused',
        '-Wduplicated-cond',
        '-Wduplicated-branches',
        '-Wlogical-op',
        '-Wnull-dereference',
        '-Wunused-parameter',
        '-Wdouble-promotion',
        '-Wshadow',
        '-Wno-psabi',
        '-Wno-attributes',
        language: 'cpp',
    )
endif

if (get_option('buildtype') != 'plain')
    if (get_option('b_lto') == true and get_option('optimization') != '0')
        # Reduce the binary size by removing unnecessary
        # dynamic symbol table entries

        add_project_arguments(
            cxx.get_supported_arguments(
                [
                    '-fno-fat-lto-objects',
                    '-fvisibility=hidden',
                    '-fvisibility-inlines-hidden',
                ],
            ),
            language: 'cpp',
        )

        if cxx.has_link_argument('-Wl,--exclude-libs,ALL')
            add_project_link_arguments('-Wl,--exclude-libs,ALL', language: 'cpp')
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
    '-Wformat-security',
]

## Add security flags for builds of type 'release','debugoptimized' and 'minsize'
plain = get_option('buildtype') == 'plain'
debug = get_option('buildtype').startswith('debug')
if not (plain or debug)
    add_project_arguments(
        cxx.get_supported_arguments([security_flags]),
        language: 'cpp',
    )
endif

# Boost dependency configuration

add_project_arguments(
    cxx.get_supported_arguments(
        [
            '-DBOOST_ALL_NO_LIB',
            '-DBOOST_ALLOW_DEPRECATED_HEADERS',
            '-DBOOST_ASIO_DISABLE_THREADS',
            '-DBOOST_ASIO_NO_DEPRECATED',
            '-DBOOST_ASIO_SEPARATE_COMPILATION',
            '-DBOOST_BEAST_SEPARATE_COMPILATION',
            '-DBOOST_EXCEPTION_DISABLE',
            '-DBOOST_NO_EXCEPTIONS',
            '-DBOOST_URL_NO_SOURCE_LOCATION',
            '-DBOOST_SPIRIT_X3_NO_RTTI',
            '-DJSON_NOEXCEPTION',
            '-DJSON_NO_IO',
            '-DJSON_USE_IMPLICIT_CONVERSIONS=0',
            '-DOPENSSL_NO_FILENAMES',
            '-DSDBUSPLUS_DISABLE_BOOST_COROUTINES',
        ],
    ),
    language: 'cpp',
)

# Find the dependency modules, if not found use meson wrap to get them
# automatically during the configure step
bmcweb_dependencies = []

pam = cxx.find_library('pam', required: true)
atomic = cxx.find_library('atomic', required: true)
bmcweb_dependencies += [pam, atomic]

openssl = dependency('openssl', required: false, version: '>=3.0.0')
if not openssl.found()
    openssl_proj = subproject(
        'openssl',
        required: true,
        default_options: ['warning_level=0', 'werror=false'],
    )
    openssl = openssl_proj.get_variable('openssl_dep')
    openssl = openssl.as_system('system')
endif
bmcweb_dependencies += [openssl]

zstd = dependency('libzstd', required: get_option('http-zstd').allowed())
if zstd.found()
    add_project_arguments('-DHAVE_ZSTD', language: 'cpp')
    bmcweb_dependencies += [zstd]
endif
nghttp2 = dependency('libnghttp2', version: '>=1.52.0', required: false)
if not nghttp2.found()
    cmake = import('cmake')
    opt_var = cmake.subproject_options()
    opt_var.add_cmake_defines(
        {
            'CMAKE_C_FLAGS': ' '.join(nghttp2_flags),
            'CMAKE_CXX_FLAGS': ' '.join(nghttp2_flags),
            'ENABLE_LIB_ONLY': true,
            'ENABLE_STATIC_LIB': true,
        },
    )
    nghttp2_ex = cmake.subproject('nghttp2', options: opt_var)
    nghttp2 = nghttp2_ex.dependency('nghttp2')
endif
bmcweb_dependencies += nghttp2

sdbusplus = dependency('sdbusplus')
bmcweb_dependencies += sdbusplus

cli11 = dependency('CLI11', required: false, include_type: 'system')
if not cli11.found()
    cli11_proj = subproject('cli11', required: true)
    cli11 = cli11_proj.get_variable('CLI11_dep')
    cli11 = cli11.as_system('system')
endif
bmcweb_dependencies += cli11

tinyxml = dependency(
    'tinyxml2',
    include_type: 'system',
    version: '>=9.0.0',
    default_options: ['tests=false'],
)
if not tinyxml.found()
    tinyxml_proj = subproject('tinyxml2', required: true)
    tinyxml = tinyxml_proj.get_variable('tinyxml_dep')
    tinyxml = tinyxml.as_system('system')
endif
bmcweb_dependencies += tinyxml

systemd = dependency('systemd')
libsystemd = dependency('libsystemd')
add_project_arguments(
    '-DLIBSYSTEMD_VERSION=' + libsystemd.version(),
    language: 'cpp',
)

zlib = dependency('zlib')
bmcweb_dependencies += [libsystemd, zlib]

nlohmann_json_dep = dependency(
    'nlohmann_json',
    version: '>=3.11.3',
    include_type: 'system',
)
bmcweb_dependencies += nlohmann_json_dep

boost = dependency(
    'boost',
    modules: ['url', 'process'],
    version: '>=1.89.0',
    static: true,
    required: false,
    include_type: 'system',
)

if boost.found()
    bmcweb_dependencies += [boost]
else
    cmake = import('cmake')
    opt = cmake.subproject_options()
    boost_libs = [
        'asio',
        'beast',
        'circular_buffer',
        'callable_traits',
        'headers',
        'process',
        'type_index',
        'url',
        'uuid',
        'spirit',
    ]
    opt.add_cmake_defines(
        {
            'CMAKE_CXX_FLAGS': ' '.join(boost_flags),
            'CMAKE_C_FLAGS': ' '.join(boost_flags),
            'BOOST_INCLUDE_LIBRARIES': ';'.join(boost_libs),
            'BUILD_SHARED_LIBS': 'OFF',
            'BOOST_PROCESS_USE_STD_FS': 'ON',
        },
    )

    boost = cmake.subproject('boost', required: true, options: opt)
    foreach boost_lib : boost_libs
        boost_lib_instance = boost.dependency('boost_' + boost_lib).as_system()
        bmcweb_dependencies += [boost_lib_instance]
    endforeach
endif

systemd_system_unit_dir = systemd.get_variable('systemd_system_unit_dir')

bindir = get_option('prefix') + '/' + get_option('bindir')

summary(
    {
        'prefix': get_option('prefix'),
        'bindir': bindir,
        'systemd unit directory': systemd_system_unit_dir,
    },
    section: 'Directories',
)

subdir('static')
subdir('redfish-core')

# Config subdirectory
subdir('config')
bmcweb_dependencies += conf_h_dep

test_sources = []
subdir('features')

# Source files
fs = import('fs')

srcfiles_bmcweb = files(
    'http/mutual_tls.cpp',
    'http/routing/sserule.cpp',
    'http/routing/websocketrule.cpp',
    'http/zstd_decompressor.cpp',
    'redfish-core/src/dbus_log_watcher.cpp',
    'redfish-core/src/error_message_utils.cpp',
    'redfish-core/src/error_messages.cpp',
    'redfish-core/src/event_log.cpp',
    'redfish-core/src/filesystem_log_watcher.cpp',
    'redfish-core/src/filter_expr_executor.cpp',
    'redfish-core/src/filter_expr_printer.cpp',
    'redfish-core/src/heartbeat_messages.cpp',
    'redfish-core/src/redfish.cpp',
    'redfish-core/src/registries.cpp',
    'redfish-core/src/resource_messages.cpp',
    'redfish-core/src/subscription.cpp',
    'redfish-core/src/task_messages.cpp',
    'redfish-core/src/update_messages.cpp',
    'redfish-core/src/utils/dbus_utils.cpp',
    'redfish-core/src/utils/json_utils.cpp',
    'redfish-core/src/utils/time_utils.cpp',
    'src/boost_asio.cpp',
    'src/boost_asio_ssl.cpp',
    'src/boost_beast.cpp',
    'src/dbus_singleton.cpp',
    'src/dbus_utility.cpp',
    'src/json_html_serializer.cpp',
    'src/ossl_random.cpp',
    'src/ssl_key_handler.cpp',
    'src/webserver_cli.cpp',
    'src/webserver_run.cpp',
)

bmcweblib = static_library(
    'bmcweblib',
    srcfiles_bmcweb,
    include_directories: incdir,
    dependencies: bmcweb_dependencies,
)

# Generate the bmcweb daemon
executable(
    'bmcweb',
    'src/webserver_main.cpp',
    include_directories: incdir,
    dependencies: bmcweb_dependencies,
    link_with: bmcweblib,
    link_args: '-Wl,--gc-sections',
    install: true,
    install_dir: bindir,
)

subdir('test')
