// SPDX-License-Identifier: GPL-2.0-or-later
// gt2ap.cfg: what the client remembers between runs, as key=value lines.
#pragma once

#include <string>

// Empty when the file doesn't have it.
std::string Cfg_Get(const std::string &key);
// Saves at once.
void Cfg_Set(const std::string &key, const std::string &value);
