# Schemas to install: name -> json-schema version
schemas_to_install = {
    'OpenBMCAccountService': 'v1_0_0',
    'OpenBMCManager': 'v1_1_0',
}

# Mapping from option key name to schemas that should be installed if that option is enabled
schemas = {
    'redfish-provisioning-feature': 'OpenBMCComputerSystem',
    #'vm-nbdproxy': 'OpenBMCVirtualMedia',
}

foreach option_key, schema : schemas
    if get_option(option_key).allowed()
        schemas_to_install += {schema: 'v1_0_0'}
    endif
endforeach


foreach schema, version : schemas_to_install
    install_data(
        'csdl/@0@_v1.xml'.format(schema),
        install_dir: 'share/www/redfish/v1/schema',
        follow_symlinks: true,
    )

    install_data(
        'json-schema/@0@.@1@.json'.format(schema, version),
        install_dir: 'share/www/redfish/v1/JsonSchemas',
        follow_symlinks: true,
    )
endforeach
