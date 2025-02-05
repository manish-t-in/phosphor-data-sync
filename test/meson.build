gtest_dep = dependency('gtest', main : true, required : false)
gmock_dep = dependency('gmock', required: false)
if not gtest_dep.found()
    gtest_proj = import('cmake').subproject('googletest', required: false)
    if gtest_proj.found()
        gtest_dep = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ]
        )
        gmock_dep = gtest_proj.dependency('gmock')
    else
        assert(
            not get_option('tests').enabled(),
            'Googletest is required if tests are enabled'
        )
    endif
endif

test_source_files = [
        'data_sync_config_test',
        'manager_test',
        'periodic_sync_test',
        'full_sync_test',
    ]

foreach test_file : test_source_files
    test(
        'test_' + test_file.underscorify(),
        executable(
            'test-' + test_file.underscorify(),
            test_file + '.cpp',
            rbmc_data_sync_sources,
            dependencies : [
                gtest_dep,
                gmock_dep,
                rbmc_data_sync_dependencies,
            ],
            include_directories: inc_dir,
            cpp_args : ['-DUNIT_TEST'],
        )
    )
endforeach
