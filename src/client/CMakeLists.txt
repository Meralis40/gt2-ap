# SPDX-License-Identifier: GPL-2.0-or-later
add_executable(gt2ap main.cpp)
target_link_libraries(gt2ap PRIVATE gt2ap::discio)
target_compile_definitions(gt2ap PRIVATE GT2AP_VERSION="${PROJECT_VERSION}")
gt2ap_set_warnings(gt2ap)
