# IBM schemas that should be installed
schemas = [
    'IBMComputerSystem',
    'IBMFabricAdapter',
    'IBMLogEntryAttachment',
    'IBMManager',
    'IBMManagerAccount',
    'IBMPCIeDevice',
    'IBMPCIeSlots',
    'IBMServiceRoot',
    'OemUpdateService',
]

foreach schema : schemas
    install_data(
        'csdl/@0@_v1.xml'.format(schema),
        install_dir: 'share/www/redfish/v1/schema',
        follow_symlinks: true,
    )

    install_data(
        'json-schema/@0@.v1_0_0.json'.format(schema),
        install_dir: 'share/www/redfish/v1/JsonSchemas',
        follow_symlinks: true,
    )
endforeach
