
if get_option('redfish-csdl').allowed()
    install_subdir(
        'installed',
        install_dir: 'share/www/redfish/v1/schema',
        strip_directory: true,
        follow_symlinks: true,
    )
endif

if get_option('redfish-jsonschema').allowed()
    install_subdir(
        'json-schema-installed',
        install_dir: 'share/www/redfish/v1/JsonSchemas',
        strip_directory: true,
        follow_symlinks: true,
    )
endif
