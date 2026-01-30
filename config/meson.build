# Gather the Configuration data

conf_data = configuration_data()

feature_options = [
    'basic-auth',
    'cookie-auth',
    'experimental-bmcweb-user',
    'experimental-redfish-dbus-log-subscription',
    'experimental-redfish-multi-computer-system',
    'google-api',
    'host-serial-socket',
    'http-zstd',
    'http2',
    'hypervisor-computer-system',
    'ibm-management-console',
    'insecure-disable-auth',
    'insecure-disable-csrf',
    'insecure-disable-ssl',
    'insecure-enable-redfish-query',
    'insecure-ignore-content-type',
    'insecure-push-style-notification',
    'kvm',
    'mutual-tls-auth',
    'redfish',
    'redfish-aggregation',
    'redfish-allow-deprecated-indicatorled',
    'redfish-allow-deprecated-power-thermal',
    'redfish-allow-simple-update',
    'redfish-bmc-journal',
    'redfish-cpu-log',
    'redfish-dbus-log',
    'redfish-dump-log',
    'redfish-host-logger',
    'redfish-new-powersubsystem-thermalsubsystem',
    'redfish-oem-manager-fan-data',
    'redfish-provisioning-feature',
    'redfish-updateservice-use-dbus',
    'redfish-use-hardcoded-system-location-indicator',
    'rest',
    'session-auth',
    'static-hosting',
    'tests',
    'vm-websocket',
    'xtoken-auth',
]

string_options = [
    'dns-resolver',
    'mutual-tls-common-name-parsing-default',
    'redfish-manager-uri-name',
    'redfish-system-uri-name',
    'redfish-eventlog-location',
]

int_options = ['http-body-limit', 'watchdog-timeout-seconds']

feature_options_string = '\n// Feature options\n'
string_options_string = '\n// String options\n'
int_options_string = '\n// Integer options\n'


foreach feature_type, feature_list : {
    'Feature': feature_options,
    'Numeric': int_options,
    'String': string_options,
}
    summary_dict = {}
    foreach option_key : feature_list
        option_key_config = 'BMCWEB_' + option_key.to_upper()
        option_key_config = option_key_config.replace('-', '_')

        opt = get_option(option_key)
        if feature_type == 'Feature'
            opt = opt.allowed()
            feature_options_string += 'constexpr const bool @0@=@1@;\n'.format(
                option_key_config,
                opt.to_string(),
            )
        endif
        if feature_type == 'Numeric'
            int_options_string += 'constexpr const int @0@=@1@;\n'.format(
                option_key_config,
                opt.to_string(),
            )
        endif
        if feature_type == 'String'
            string_options_string += 'constexpr std::string_view @0@="@1@";\n'.format(
                option_key_config,
                opt,
            )
        endif
        summary_dict += {option_key: opt}
    endforeach
    summary(summary_dict, section: feature_type + ' Options', bool_yn: true)
endforeach

# Logging level
loglvlopt = get_option('bmcweb-logging')
if get_option('buildtype').startswith('debug') and loglvlopt == 'disabled'
    # Override logging level as 'debug' if 'bmcweb-logging' is set as 'disabled'
    loglvlopt = 'debug'
endif
loglvlopt = loglvlopt.to_upper()
string_options_string += 'constexpr std::string_view  BMCWEB_LOGGING_LEVEL' + ' = "' + loglvlopt + '";\n'

# NBD proxy is disabled due to lack of maintenance.  See meson_options.txt
feature_options_string += 'constexpr const bool BMCWEB_VM_NBDPROXY = false;\n'

conf_data.set(
    'BMCWEB_OPTIONS',
    string_options_string + int_options_string + feature_options_string,
)

conf_h_dep = declare_dependency(
    include_directories: include_directories('.'),
    sources: configure_file(
        input: 'bmcweb_config.h.in',
        output: 'bmcweb_config.h',
        configuration: conf_data,
    ),
)

# Configure and install systemd unit files
https_port = get_option('https_port')
if https_port > 0
    http_level_allowed = 'https'
    if get_option('insecure-disable-ssl').allowed()
        http_level_allowed = 'http'
        if https_port == 443
          https_port = 80
        endif
    endif
    configure_file(
        input: 'bmcweb.socket.in',
        output: 'bmcweb.socket',
        install_dir: systemd_system_unit_dir,
        install: true,
        configuration: configuration_data(
            {
                'BMCWEB_PORT': https_port,
                'HTTP_LEVEL_ALLOWED': http_level_allowed,
                'HTTP_AUTH_LEVEL': 'auth',
                'HTTP_BIND': '',
            },
        ),
    )
endif

ports = get_option('additional-ports')
binds = get_option('additional-bind-to-device')
auths = get_option('additional-auth')
foreach index : range(ports.length())
    port_number = ports[index]
    bind_to_device = '0.0.0.0'
    auth = 'auth'
    if index < binds.length()
        bind_to_device = binds[index]
    endif

    if index < auths.length()
        auth = auths[index]
    endif

    filename = 'bmcweb_' + port_number + '.socket'
    configure_file(
        input: 'bmcweb.socket.in',
        output: filename,
        install_dir: systemd_system_unit_dir,
        install: true,
        configuration: configuration_data(
            {
                'BMCWEB_PORT': port_number,
                'HTTP_LEVEL_ALLOWED': 'https',
                'HTTP_BIND': bind_to_device,
                'HTTP_AUTH_LEVEL': auth,
            },
        ),
    )
endforeach

if get_option('experimental-bmcweb-user').allowed()
    user = 'bmcweb'
    group = 'bmcweb'
    dynamic_user = 'yes'
    state_directory = 'bmcweb'
    work_dir = '-~'
else
    user = 'root'
    group = 'root'
    dynamic_user = 'no'
    state_directory = '/home/root'
    work_dir = '/home/root'
endif

configure_file(
    input: 'bmcweb.service.in',
    output: 'bmcweb.service',
    install_dir: systemd_system_unit_dir,
    install: true,
    configuration: configuration_data(
        {
            'MESON_INSTALL_PREFIX': get_option('prefix'),
            'BMCWEB_USER': user,
            'BMCWEB_GROUP': group,
            'BMCWEB_STATE_DIRECTORY': state_directory,
            'BMCWEB_DYNAMICUSER': dynamic_user,
            'BMCWEB_WORKING_DIRECTORY': work_dir,
            'BMCWEB_WATCHDOG_TIMEOUT_SECONDS': get_option(
                'watchdog-timeout-seconds',
            ),
        },
    ),
)

# Copy pam-webserver to etc/pam.d
install_data('pam-webserver', install_dir: '/etc/pam.d/', rename: 'webserver')
