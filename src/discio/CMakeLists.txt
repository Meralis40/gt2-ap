# SPDX-License-Identifier: GPL-2.0-or-later
add_library(discio STATIC
    cue.cpp
    disc_image.cpp
    iso9660.cpp
    sha1.cpp
    game_id.cpp
    gzip.cpp
    overlay.cpp
)
add_library(gt2ap::discio ALIAS discio)
target_include_directories(discio PUBLIC include)
gt2ap_set_warnings(discio)
