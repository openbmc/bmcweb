# Gather the Configuration data

conf_data = configuration_data()

feature_options = [
    'basic-auth',
    'cookie-auth',
    'dns-resolver',
    'event-subscription',
    'experimental-http2',
    'experimental-redfish-multi-computer-system',
    'google-api',
    'host-serial-socket',
    'http-body-limit',
    'https_port',
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
    'mutual-tls-common-name-parsing',
    'redfish-aggregation',
    'redfish-allow-deprecated-power-thermal',
    'redfish-bmc-journal',
    'redfish-cpu-log',
    'redfish-dbus-log',
    'redfish-dump-log',
    'redfish-host-logger',
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
]

int_options = [
    'http-body-limit',
    'https_port',
]

foreach option_key : feature_options

    option_key_config = option_key.to_upper()
    option_key_config = option_key_config.replace('-', '_')

    message(option_key_config)
    opt = get_option(option_key)
    if string_options.contains(option_key)
    elif int_options.contains(option_key)
    else
        opt = opt.allowed().to_string()
    endif
    conf_data.set(option_key_config, opt)
    summary(option_key, opt, section: 'Features')
endforeach

conf_data.set('MESON_INSTALL_PREFIX', get_option('prefix'))
enable_health_populate = get_option('redfish-health-populate')
conf_data.set10('BMCWEB_ENABLE_HEALTH_POPULATE', enable_health_populate.allowed())
enable_license_status = get_option('redfish-license')
conf_data.set10('BMCWEB_ENABLE_REDFISH_LICENSE', enable_license_status.allowed())
conf_data.set10('BMCWEB_VIRTUAL_MEDIA_VM', get_option('vm-websocket').allowed())
conf_data.set10('BMCWEB_VIRTUAL_MEDIA_NBD', false)

# Logging level
loglvlopt = get_option('bmcweb-logging')
if get_option('buildtype').startswith('debug') and loglvlopt == 'disabled'
    # Override logging level as 'debug' if 'bmcweb-logging' is set as 'disabled'
    loglvlopt = 'debug'
endif
loglvlopt = loglvlopt.to_upper()
conf_data.set('LOGGING_LEVEL', loglvlopt)

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
    configuration: conf_data,
    install: true,
)

configure_file(
    input: 'bmcweb.service.in',
    output: 'bmcweb.service',
    install_dir: systemd_system_unit_dir,
    configuration: conf_data,
    install: true,
)

# Copy pam-webserver to etc/pam.d
install_data(
    'pam-webserver',
    install_dir: '/etc/pam.d/',
    rename: 'webserver',
)
