copyclip_add_test(logging_test SOURCES LoggingTest.cpp LIBS copyclip::runtime
                  copyclip::config)
copyclip_add_test(single_instance_guard_test SOURCES SingleInstanceGuardTest.cpp LIBS
                  copyclip::runtime)
