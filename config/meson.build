# Gather the Configuration data

conf_data = configuration_data()

feature_options = [
    'basic-auth',
    'cookie-auth',
    'experimental-http2',
    'experimental-redfish-multi-computer-system',
    'experimental-redfish-dbus-log-subscription',
    'google-api',
    'host-serial-socket',
    'hypervisor-computer-system',
    'ibm-management-console',
    'insecure-disable-auth',
    'insecure-disable-csrf',
    'insecure-disable-ssl',
    'insecure-enable-redfish-query',
    'insecure-ignore-content-type',
    'insecure-push-style-notification',
    'kvm',
    'meta-tls-common-name-parsing',
    'mutual-tls-auth',
    'redfish-aggregation',
    'redfish-allow-deprecated-power-thermal',
    'redfish-allow-simple-update',
    'redfish-use-3-digit-messageid',
    'redfish-bmc-journal',
    'redfish-cpu-log',
    'redfish-dbus-log',
    'redfish-dump-log',
    'redfish-host-logger',
    'redfish-new-powersubsystem-thermalsubsystem',
    'redfish-oem-manager-fan-data',
    'redfish-provisioning-feature',
    'redfish-updateservice-use-dbus',
    'redfish',
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
]

int_options = ['http-body-limit']

feature_options_string = '\n//Feature options\n'
string_options_string = '\n// String options\n'
int_options_string = '\n// Integer options\n'

foreach option_key : feature_options + string_options + int_options
    option_key_config = 'BMCWEB_' + option_key.to_upper()
    option_key_config = option_key_config.replace('-', '_')

    message(option_key_config)

    opt = get_option(option_key)
    if string_options.contains(option_key)
        string_options_string += 'constexpr std::string_view  ' + option_key_config + ' = "' + opt + '";\n'
    elif int_options.contains(option_key)
        int_options_string += 'constexpr const int         ' + option_key_config + ' = ' + opt.to_string() + ';\n'
    else
        feature_options_string += 'constexpr const bool        ' + option_key_config + ' = ' + opt.allowed().to_string() + ';\n'
        opt = opt.allowed().to_string()
    endif
    summary(option_key, opt, section: 'Features')
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
feature_options_string += 'constexpr const bool        BMCWEB_VM_NBDPROXY = false;\n'

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
    configure_file(
        input: 'bmcweb.socket.in',
        output: 'bmcweb.socket',
        install_dir: systemd_system_unit_dir,
        install: true,
        configuration: configuration_data(
            {
                'BMCWEB_PORT': https_port,
                'HTTP_LEVEL_ALLOWED': 'https',
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

    filename = 'bmcweb_' + port_number
    configure_file(
        input: 'bmcweb.socket.in',
        output: filename,
        install_dir: systemd_system_unit_dir,
        install: true,
        configuration: configuration_data(
            {
                'BMCWEB_HTTPS_PORT': port_number,
                'HTTP_LEVEL_ALLOWED': 'https',
                'HTTP_BIND': bind_to_device,
                'HTTP_AUTH_LEVEL': auth,
            },
        ),
    )
endforeach

configure_file(
    input: 'bmcweb.service.in',
    output: 'bmcweb.service',
    install_dir: systemd_system_unit_dir,
    install: true,
    configuration: configuration_data(
        {'MESON_INSTALL_PREFIX': get_option('prefix')},
    ),
)

# Copy pam-webserver to etc/pam.d
install_data('pam-webserver', install_dir: '/etc/pam.d/', rename: 'webserver')
