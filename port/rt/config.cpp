// SPDX-License-Identifier: GPL-2.0-or-later
#include "config.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace {

const std::filesystem::path kConfigFile = "gt2ap.cfg";

// In the file's order, with any key this version doesn't know, for the next save.
std::vector<std::pair<std::string, std::string>> g_Lines;
bool g_Loaded;

void load()
{
    if (g_Loaded) {
        return;
    }
    g_Loaded = true;
    std::ifstream in(kConfigFile);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        auto equals = line.find('=');
        if (equals != std::string::npos) {
            g_Lines.emplace_back(line.substr(0, equals), line.substr(equals + 1));
        }
    }
}

}  // namespace

std::string Cfg_Get(const std::string &key)
{
    load();
    for (const auto &[name, value] : g_Lines) {
        if (name == key) {
            return value;
        }
    }
    return {};
}

void Cfg_Set(const std::string &key, const std::string &value)
{
    load();
    bool found = false;
    for (auto &[name, current] : g_Lines) {
        if (name == key) {
            current = value;
            found = true;
        }
    }
    if (!found) {
        g_Lines.emplace_back(key, value);
    }
    std::ofstream out(kConfigFile);
    for (const auto &[name, current] : g_Lines) {
        out << name << "=" << current << "\n";
    }
}
