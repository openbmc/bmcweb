# Mapping from option key name to schemas that should be installed if that option is enabled
schemas = {
    'insecure-disable-auth': {
        'csdl': 'OpenBMCAccountService',
        'json': 'OpenBMCAccountService.v1_0_0',
    },
    'redfish-oem-manager-fan-data': {
        'csdl': 'OpenBMCManager',
        'json': 'OpenBMCManager.v1_1_0',
    },
    'redfish-provisioning-feature': {
        'csdl': 'OpenBMCComputerSystem',
        'json': 'OpenBMCComputerSystem.v1_0_0',
    },
    #'vm-nbdproxy': {
    #  'csdl': 'OpenBMCVirtualMedia',
    #  'json': 'OpenBMCVirtualMedia.v1_0_0',
    #},
}

foreach option_key, info : schemas
    if get_option(option_key).allowed()
        install_data(
            'csdl/@0@_v1.xml'.format(info['csdl']),
            install_dir: 'share/www/redfish/v1/schema',
            follow_symlinks: true,
        )
        install_data(
            'json-schema/@0@.json'.format(info['json']),
            install_dir: 'share/www/redfish/v1/JsonSchemas',
            follow_symlinks: true,
        )
    endif
endforeach
