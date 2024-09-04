# Gather the Configuration data

conf_data = configuration_data()

feature_options = [
    'audit-events',
    'basic-auth',
    'bmc-shell-socket',
    'cookie-auth',
    'event-subscription',
    'experimental-http2',
    'experimental-redfish-multi-computer-system',
    'google-api',
    'host-serial-socket',
    'hw-isolation',
    'hypervisor-serial-socket',
    'ibm-led-extensions',
    'ibm-management-console',
    'ibm-usb-code-update',
    'insecure-disable-auth',
    'insecure-disable-csrf',
    'insecure-disable-ssl',
    'insecure-enable-redfish-query',
    'insecure-ignore-content-type',
    'insecure-push-style-notification',
    'insecure-tftp-update',
    'kvm',
    'mutual-tls-auth',
    'redfish-aggregation',
    'redfish-allow-deprecated-power-thermal',
    'redfish-bmc-journal',
    'redfish-cpu-log',
    'redfish-dbus-log',
    'redfish-dump-log',
    'redfish-health-populate',
    'redfish-host-logger',
    'redfish-license',
    'redfish-new-powersubsystem-thermalsubsystem',
    'redfish-oem-manager-fan-data',
    'redfish-provisioning-feature',
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
    'mutual-tls-common-name-parsing',
    'redfish-manager-uri-name',
    'redfish-system-uri-name',
]

int_options = [
    'http-body-limit',
]

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
configure_file(
    input: 'bmcweb.socket.in',
    output: 'bmcweb.socket',
    install_dir: systemd_system_unit_dir,
    install: true,
    configuration: configuration_data({
        'BMCWEB_HTTPS_PORT': get_option('https_port'),
    }),
)

configure_file(
    input: 'bmcweb.service.in',
    output: 'bmcweb.service',
    install_dir: systemd_system_unit_dir,
    install: true,
    configuration: configuration_data({
        'MESON_INSTALL_PREFIX': get_option('prefix'),
    }),
)

# Copy pam-webserver to etc/pam.d
install_data(
    'pam-webserver',
    install_dir: '/etc/pam.d/',
    rename: 'webserver',
)
