// SPDX-License-Identifier: GPL-2.0-or-later
// The host side of a session: what the player has received, which locations are done, and
// the files that go with a seed. Only the game's thread touches this, in Ap_Poll and the calls
// from game code. IDs and item tables follow world/gt2 (items.py, locations.py).
#include "ap.h"
#include "ap_host.h"

#include "hw.h"
#include "rt.h"
#include "session.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

constexpr long long kItemLicense = 1, kItemTrophy = 2, kItemManufacturers = 100, kItemParts = 200, kItemCredits = 300;
constexpr long long kLocationEvents = 1000, kLocationTests = 2000, kLocationLicenses = 2100;
constexpr int kCredits[] = {3000, 5000, 10000, 20000, 35000, 50000, 75000};
constexpr int kManufacturers = 64;
constexpr int kParts = 64;
constexpr int kTiers = 6;  // tier 1 is license B, whose LicenseId is 5
constexpr int kGoalEvents[] = {137, 138, 139, 140, 141};  // the GT World League
constexpr int kMaxToasts = 4;
constexpr auto kToastTime = std::chrono::seconds(8);

struct PartStage {
    std::vector<std::vector<int>> stages;  // the PartIds each copy unlocks, in order
};

const std::vector<PartStage> kPartItems = {
    {{{46}, {47}, {48}, {49}}},  // Progressive Turbo
    {{{29}, {30}, {31}}},        // Progressive NA Tune
    {{{26}, {27}, {28}}},        // Progressive Muffler
    {{{16}, {17}}},              // Progressive Intercooler
    {{{18}, {19}, {20}, {34}}},  // Progressive Weight Reduction
    {{{6}}},                     // Performance Chip
    {{{7}}},                     // Displacement Increase
    {{{9}}},                     // Engine Balancing
    {{{32}}},                    // Port & Polish
    {{{45}}},                    // Dirt Tires
    {{{3}, {4}, {5}}},           // Progressive Clutch
    {{{10}, {11}, {12}}},        // Progressive Flywheel
    {{{13}, {14}, {15}}},        // Progressive Transmission
    {{{35}, {36}, {37}}},        // Progressive Suspension
    {{{39}, {40}, {41}, {42}, {43}}},  // Progressive Racing Tires
    {{{44}}},                    // Simulation Tires
    {{{21, 22, 23, 24, 25}}},    // Limited-Slip Differential
    {{{1}}},                     // Sports Brakes
    {{{2}}},                     // Brake Balance Controller
    {{{33}}},                    // Carbon Driveshaft
    {{{0}}},                     // Active Stability Controller
    {{{38}}},                    // TCS Controller
};

struct Toast {
    std::string text;
    Clock::time_point until;
};

bool g_Active;
bool g_PageDisabled;
bool g_AutoConnect;
bool g_Started;
int g_PageState = AP_PAGE_IDLE;
std::string g_Status;
std::string g_Fields[3];
int g_EditField = -1;

std::vector<long long> g_Received;
int g_Licenses, g_Trophies;
bool g_Manufacturers[kManufacturers];
bool g_Parts[kParts];
int g_TrophiesOn, g_ProNeeded, g_WorldNeeded;

std::set<long long> g_Done;
std::filesystem::path g_LocationsFile;
bool g_GoalSent;
int g_RaceEvents[6];
std::deque<Toast> g_Toasts;

const std::filesystem::path kConfigFile = "gt2ap.cfg";
const std::filesystem::path kSaveDir = "saves";

void toast(const std::string &text)
{
    Rt_Log("AP: %s", text.c_str());
    if (g_Toasts.size() == kMaxToasts) {
        g_Toasts.pop_front();
    }
    g_Toasts.push_back({text, Clock::now() + kToastTime});
}

void loadConfig()
{
    std::ifstream in(kConfigFile);
    std::string line;

    while (std::getline(in, line)) {
        if (line.rfind("server=", 0) == 0) {
            g_Fields[0] = line.substr(7);
        } else if (line.rfind("slot=", 0) == 0) {
            g_Fields[1] = line.substr(5);
        }
    }
}

// The password isn't kept.
void saveConfig()
{
    std::ofstream out(kConfigFile);
    out << "server=" << g_Fields[0] << "\nslot=" << g_Fields[1] << "\n";
}

void rebuildFromItems()
{
    g_Licenses = g_Trophies = 0;
    std::fill(std::begin(g_Manufacturers), std::end(g_Manufacturers), false);
    std::fill(std::begin(g_Parts), std::end(g_Parts), false);
    std::vector<int> copies(kPartItems.size());
    for (long long item : g_Received) {
        if (item == kItemLicense) {
            g_Licenses++;
        } else if (item == kItemTrophy) {
            g_Trophies++;
        } else if (item >= kItemManufacturers && item < kItemManufacturers + kManufacturers) {
            g_Manufacturers[item - kItemManufacturers] = true;
        } else if (item >= kItemParts && item < kItemParts + static_cast<long long>(kPartItems.size())) {
            auto index = static_cast<std::size_t>(item - kItemParts);
            int copy = copies[index]++;
            const auto &stages = kPartItems[index].stages;
            if (copy < static_cast<int>(stages.size())) {
                for (int part : stages[copy]) {
                    g_Parts[part] = true;
                }
            }
        }
    }
}

void checkGoal()
{
    if (g_GoalSent) {
        return;
    }
    for (int event : kGoalEvents) {
        if (g_Done.count(kLocationEvents + event) == 0) {
            return;
        }
    }
    g_GoalSent = true;
    ap::Session_SendGoal();
}

void location(long long id)
{
    if (!g_Active || !g_Done.insert(id).second) {
        return;
    }
    // On file first, so a check made while the server is away isn't lost with the client.
    std::ofstream(g_LocationsFile, std::ios::app) << id << "\n";
    ap::Session_SendLocations({id});
    checkGoal();
}

std::string fileName(std::string text)
{
    for (char &c : text) {
        bool plain = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!plain) {
            c = '_';
        }
    }
    return text;
}

// The first connection of the run picks the seed's files.
void openSeed(const std::string &seed)
{
    std::filesystem::create_directories(kSaveDir);
    std::string stem = fileName(seed) + "_" + fileName(g_Fields[1]);
    g_LocationsFile = kSaveDir / (stem + ".locations");
    std::ifstream in(g_LocationsFile);
    long long id;
    while (in >> id) {
        g_Done.insert(id);
    }
    Hw_CardUseFile((kSaveDir / (stem + ".mcd")).string());
}

void onEvent(const ap::Event &event)
{
    switch (event.kind) {
    case ap::Event::kConnected:
        if (!g_Active) {
            openSeed(event.text);
            g_TrophiesOn = static_cast<int>(event.numbers[0]);
            g_ProNeeded = static_cast<int>(event.numbers[1]);
            g_WorldNeeded = static_cast<int>(event.numbers[2]);
            g_Active = true;
            g_PageState = AP_PAGE_CONNECTED;
        } else {
            toast("connected again");
        }
        if (!g_Done.empty()) {
            ap::Session_SendLocations(std::vector<long long>(g_Done.begin(), g_Done.end()));
        }
        break;
    case ap::Event::kRefused:
        g_PageState = AP_PAGE_FAILED;
        g_Status = event.text;
        g_Started = false;
        if (g_Active) {
            toast(event.text);
        }
        break;
    case ap::Event::kLost:
        if (g_Active) {
            toast("server: " + event.text + ", retrying");
        }
        break;
    case ap::Event::kItems:
        if (event.index == 0) {
            g_Received.clear();
        } else if (event.index != static_cast<int>(g_Received.size())) {
            ap::Session_RequestSync();
            break;
        }
        g_Received.insert(g_Received.end(), event.numbers.begin(), event.numbers.end());
        rebuildFromItems();
        break;
    case ap::Event::kChecked:
        for (long long id : event.numbers) {
            g_Done.insert(id);
        }
        checkGoal();
        break;
    case ap::Event::kMessage:
        toast(event.text);
        break;
    }
}

void startSession()
{
    if (g_Started) {
        return;
    }
    g_Started = true;
    g_PageState = AP_PAGE_CONNECTING;
    g_Status = "Connecting ...";
    saveConfig();
    ap::Session_Start({g_Fields[0], g_Fields[1], g_Fields[2]});
}

}  // namespace

void Ap_Poll()
{
    for (const ap::Event &event : ap::Session_TakeEvents()) {
        onEvent(event);
    }
    auto now = Clock::now();
    while (!g_Toasts.empty() && g_Toasts.front().until < now) {
        g_Toasts.pop_front();
    }
}

void Ap_SetLogin(const std::string &server, const std::string &slot, const std::string &password)
{
    g_Fields[0] = server;
    g_Fields[1] = slot;
    g_Fields[2] = password;
    g_AutoConnect = true;
}

void Ap_DisablePage()
{
    g_PageDisabled = true;
}

void Ap_ConnectNow()
{
    g_AutoConnect = false;
    startSession();
}

void Ap_PageText(const char *text)
{
    if (g_EditField < 0) {
        return;
    }
    for (; *text != 0; text++) {
        if (*text >= 0x20 && *text < 0x7f) {
            g_Fields[g_EditField] += *text;
        }
    }
}

void Ap_PageBackspace()
{
    if (g_EditField >= 0 && !g_Fields[g_EditField].empty()) {
        g_Fields[g_EditField].pop_back();
    }
}

std::string Ap_ToastLine(int index)
{
    return index < static_cast<int>(g_Toasts.size()) ? g_Toasts[index].text : std::string();
}

extern "C" {

int Ap_Active(void)
{
    return g_Active;
}

int Ap_Licenses(void)
{
    return std::min(g_Licenses, kTiers);
}

int Ap_HasManufacturer(int manufacturer)
{
    return manufacturer >= 0 && manufacturer < kManufacturers && g_Manufacturers[manufacturer];
}

int Ap_HasPart(int part)
{
    return part >= 0 && part < kParts && g_Parts[part];
}

int Ap_LeagueTrophies(void)
{
    return g_Active && g_TrophiesOn;
}

int Ap_ProLeagueOpen(void)
{
    return g_Trophies >= g_ProNeeded;
}

int Ap_WorldLeagueOpen(void)
{
    return g_Trophies >= g_WorldNeeded;
}

void Ap_SetRaceEvent(int race, int eventIndex)
{
    g_RaceEvents[race] = eventIndex;
}

void Ap_RaceWon(int race)
{
    if (g_RaceEvents[race] >= 0) {
        location(kLocationEvents + g_RaceEvents[race]);
    }
}

void Ap_LicenseTestPassed(int license, int test)
{
    location(kLocationTests + (kTiers - license) * 10 + test);
}

void Ap_LicenseEarned(int license)
{
    location(kLocationLicenses + kTiers - license);
}

int Ap_NextCredits(unsigned short *applied)
{
    if (!g_Active || *applied >= g_Received.size()) {
        return -1;
    }
    long long item = g_Received[(*applied)++];
    long long tier = item - kItemCredits;
    return tier >= 0 && tier < static_cast<long long>(std::size(kCredits)) ? kCredits[tier] : 0;
}

int Ap_PageWanted(void)
{
    return !g_PageDisabled || g_AutoConnect;
}

void Ap_PageOpen(void)
{
    if (g_Fields[0].empty()) {
        loadConfig();
    }
    if (g_AutoConnect) {
        startSession();
    }
    Host_TextMode(1);
}

void Ap_PageClose(void)
{
    g_EditField = -1;
    Host_TextMode(0);
}

void Ap_PageSelect(int field)
{
    g_EditField = field;
}

void Ap_PageConnect(void)
{
    startSession();
}

const char *Ap_PageField(int field)
{
    return g_Fields[field].c_str();
}

int Ap_PageState(void)
{
    return g_PageState;
}

const char *Ap_PageStatus(void)
{
    return g_Status.c_str();
}

}  // extern "C"
