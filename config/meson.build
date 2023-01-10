# Gather the Configuration data

conf_data = configuration_data()
conf_data.set('BMCWEB_HTTP_REQ_BODY_LIMIT_MB', get_option('http-body-limit'))
xss_enabled = get_option('insecure-disable-xss')
conf_data.set10('BMCWEB_INSECURE_DISABLE_XSS_PREVENTION', xss_enabled.enabled())
enable_redfish_query = get_option('insecure-enable-redfish-query')
conf_data.set10('BMCWEB_INSECURE_ENABLE_QUERY_PARAMS', enable_redfish_query.enabled())
# enable_redfish_aggregation = get_option('redfish-aggregation')
# conf_data.set10('BMCWEB_ENABLE_REDFISH_AGGREGATION', enable_redfish_aggregation.enabled())
insecure_push_style_notification = get_option('insecure-push-style-notification')
conf_data.set10('BMCWEB_INSECURE_ENABLE_HTTP_PUSH_STYLE_EVENTING', insecure_push_style_notification.enabled())
conf_data.set('MESON_INSTALL_PREFIX', get_option('prefix'))
conf_data.set('HTTPS_PORT', get_option('https_port'))

# Logging level.
loglvl = 0
if not get_option('buildtype').startswith('debug')
  logging_level_map = {
    'debug': 0,
    'info' : 1,
    'warning' : 2,
    'error' : 3,
    'critical' : 4,
  }

  # get mapped logging-level to cpp flags
  loglvlopt = get_option('bmcweb-logging')
  if logging_level_map.has_key(loglvlopt)
    loglvl = logging_level_map.get(loglvlopt)
  endif
endif
conf_data.set('BMCWEB_LOGGING_LEVEL', loglvl)

conf_h_dep = declare_dependency(
    include_directories: include_directories('.'),
    sources: configure_file(
        input: 'bmcweb_config.h.in',
        output: 'bmcweb_config.h',
        configuration: conf_data
    )
)

# Configure and install systemd unit files

configure_file(input : 'bmcweb.socket.in',
               output : 'bmcweb.socket',
               install_dir: systemd_system_unit_dir,
               configuration: conf_data,
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
