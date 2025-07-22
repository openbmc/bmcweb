# Generate the bmcweb CLI application
executable(
    'bmcweb',
    ['webserver_cli.cpp'],
    include_directories: incdir_cli,
    dependencies: bmcweb_cli_dependencies,
    link_with: bmcweblib,
    install: true,
    install_dir: bindir,
)
