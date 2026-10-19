# SPDX-License-Identifier: GPL-2.0-or-later
add_executable(gt2-discinfo main.cpp)
target_link_libraries(gt2-discinfo PRIVATE gt2ap::discio)
gt2ap_set_warnings(gt2-discinfo)
