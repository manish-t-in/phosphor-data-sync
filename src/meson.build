# SPDX-License-Identifier: Apache-2.0

phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')
sdbusplus_dep = dependency('sdbusplus')
nlohmann_json_dep = dependency('nlohmann_json')

rbmc_data_sync_sources = [
    files(
        'data_sync_config.cpp',
        'external_data_ifaces.cpp',
        'external_data_ifaces_impl.cpp',
        'manager.cpp'
        )
  ]

rbmc_data_sync_dependencies = [
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
    conf_h_dep,
    nlohmann_json_dep,
  ]

inc_dir = include_directories('.')
libexecdir_installdir = join_paths(get_option('libexecdir'), 'phosphor-data-sync')
executable('phosphor-rbmc-data-sync-mgr',
    'rbmc_data_sync_main.cpp',
    rbmc_data_sync_sources,
    dependencies: rbmc_data_sync_dependencies,
    include_directories: inc_dir,
    install : true,
    install_dir: libexecdir_installdir
  )

systemd_system_unit_dir = dependency('systemd').get_variable(
  pkgconfig: 'systemdsystemunitdir')
rbmc_data_sync_service_unit = '../service_files/xyz.openbmc_project.Control.SyncBMCData.service'
configure_file(
  copy: true,
  input: rbmc_data_sync_service_unit,
  install : true,
  install_dir: systemd_system_unit_dir,
  output: '@PLAINNAME@'
  )
