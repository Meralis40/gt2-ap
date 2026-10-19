# SPDX-License-Identifier: GPL-2.0-or-later
add_executable(gt2ap-tests
    test_main.cpp
    test_cue.cpp
    test_sha1.cpp
    test_disc.cpp
    synthetic_disc.cpp
)
target_link_libraries(gt2ap-tests PRIVATE gt2ap::discio)
gt2ap_set_warnings(gt2ap-tests)
add_test(NAME gt2ap-tests COMMAND gt2ap-tests)
