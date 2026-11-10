// SPDX-License-Identifier: GPL-2.0-or-later
// The host side of a session: what the player has received, which locations are done, and
// the files that go with a seed. Only the game's thread touches this, in Ap_Poll and the calls
// from game code. IDs and item tables follow world/gt2 (items.py, locations.py).
#include "ap.h"
#include "ap_host.h"

#include "config.h"
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
constexpr long long kLocationEvents = 1000, kLocationTests = 2000, kLocationLicenses = 2100,
                    kLocationFinished = 3000;
constexpr int kCredits[] = {3000, 5000, 10000, 20000, 35000, 50000, 75000};
constexpr int kManufacturers = 64;
constexpr int kParts = 64;
constexpr int kTiers = 6;  // tier 1 is license B, whose LicenseId is 5
constexpr int kGoalEvents[] = {137, 138, 139, 140, 141};  // the GT World League
// The first race of each championship: Event Synthesizer Expert, GT300, GT500, GT World League.
constexpr int kChampionships[] = {83, 119, 124, 137};
constexpr int kChampionshipRaces = 5;
constexpr int kMaxToasts = 4;
constexpr std::size_t kToastWidth = 60;
constexpr auto kToastTime = std::chrono::seconds(8);

struct PartItem {
    const char *name;
    std::vector<std::vector<int>> stages;  // the PartIds each copy unlocks, in order
};

const std::vector<PartItem> kPartItems = {
    {"Progressive Turbo", {{46}, {47}, {48}, {49}}},
    {"Progressive NA Tune", {{29}, {30}, {31}}},
    {"Progressive Muffler", {{26}, {27}, {28}}},
    {"Progressive Intercooler", {{16}, {17}}},
    {"Progressive Weight Reduction", {{18}, {19}, {20}, {34}}},
    {"Performance Chip", {{6}}},
    {"Displacement Increase", {{7}}},
    {"Engine Balancing", {{9}}},
    {"Port & Polish", {{32}}},
    {"Dirt Tires", {{45}}},
    {"Progressive Clutch", {{3}, {4}, {5}}},
    {"Progressive Flywheel", {{10}, {11}, {12}}},
    {"Progressive Transmission", {{13}, {14}, {15}}},
    {"Progressive Suspension", {{35}, {36}, {37}}},
    {"Progressive Racing Tires", {{39}, {40}, {41}, {42}, {43}}},
    {"Simulation Tires", {{44}}},
    {"Limited-Slip Differential", {{21, 22, 23, 24, 25}}},
    {"Sports Brakes", {{1}}},
    {"Brake Balance Controller", {{2}}},
    {"Carbon Driveshaft", {{33}}},
    {"Active Stability Controller", {{0}}},
    {"TCS Controller", {{38}}},
};

// By manufacturer ID, with nullptr for the IDs no dealership has.
const char *const kManufacturerNames[] = {
    "Acura", "Alfa Romeo", "Aston Martin", "Audi", "BMW", "Chevrolet", "Citroen", "Daihatsu", "Dodge", "FIAT",
    "Ford", "Honda", nullptr, "Jaguar", "Lancia", nullptr, "Lister", "Lotus", "Mazda", "Mercedes-Benz",
    nullptr, "Mini & MG", "Mitsubishi", "Nissan", "Opel", "Peugeot", "Plymouth", "Renault", "RUF", "Shelby",
    "Subaru", "Suzuki", "Tommy kaira", "Toyota", "TVR", nullptr, "Vector", "Venturi", "Volkswagen",
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
std::set<long long> g_SlotLocations;
std::filesystem::path g_LocationsFile;
bool g_GoalSent;
bool g_HeldShown;
int g_RaceEvents[6];
std::deque<Toast> g_Toasts;

const std::filesystem::path kSaveDir = "saves";

void toast(const std::string &text)
{
    if (!g_Toasts.empty() && g_Toasts.back().text == text) {
        g_Toasts.back().until = Clock::now() + kToastTime;  // the same refusal, pressed again
        return;
    }
    Rt_Log("AP: %s", text.c_str());
    if (g_Toasts.size() == kMaxToasts) {
        g_Toasts.pop_front();
    }
    g_Toasts.push_back({text, Clock::now() + kToastTime});
}

void loadConfig()
{
    g_Fields[0] = Cfg_Get("server");
    g_Fields[1] = Cfg_Get("slot");
}

// The password isn't kept.
void saveConfig()
{
    Cfg_Set("server", g_Fields[0]);
    Cfg_Set("slot", g_Fields[1]);
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
            toast("Tab or right stick click: what you have unlocked");
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
    case ap::Event::kLocations:
        g_SlotLocations.insert(event.numbers.begin(), event.numbers.end());
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

ApTracker Ap_Tracker()
{
    ApTracker tracker{};

    for (int id = 0; id < static_cast<int>(std::size(kManufacturerNames)); id++) {
        if (kManufacturerNames[id] != nullptr) {
            tracker.manufacturers.push_back({kManufacturerNames[id], g_Manufacturers[id] ? 1 : 0, 1});
        }
    }
    for (const PartItem &item : kPartItems) {
        tracker.parts.push_back({item.name, 0, static_cast<int>(item.stages.size())});
    }
    for (long long item : g_Received) {
        long long credits = item - kItemCredits;
        long long part = item - kItemParts;
        if (credits >= 0 && credits < static_cast<long long>(std::size(kCredits))) {
            tracker.credits += kCredits[credits];
        } else if (part >= 0 && part < static_cast<long long>(kPartItems.size())) {
            ApTrackerEntry &entry = tracker.parts[static_cast<std::size_t>(part)];
            entry.have = std::min(entry.have + 1, entry.total);
        }
    }
    tracker.trophyRule = g_TrophiesOn != 0;
    tracker.trophies = g_Trophies;
    tracker.proNeeded = g_ProNeeded;
    tracker.worldNeeded = g_WorldNeeded;
    tracker.checksTotal = static_cast<int>(g_SlotLocations.size());
    for (long long id : g_SlotLocations) {
        tracker.checks += static_cast<int>(g_Done.count(id));
    }
    tracker.goalRacesTotal = static_cast<int>(std::size(kGoalEvents));
    for (int event : kGoalEvents) {
        tracker.goalRaces += static_cast<int>(g_Done.count(kLocationEvents + event));
    }
    return tracker;
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

void Ap_SayManufacturerLocked(int manufacturer)
{
    bool known = manufacturer >= 0 && manufacturer < static_cast<int>(std::size(kManufacturerNames)) &&
                 kManufacturerNames[manufacturer] != nullptr;
    toast(std::string(known ? kManufacturerNames[manufacturer] : "This manufacturer") + " is locked");
}

void Ap_SayPartLocked(int part)
{
    for (const PartItem &item : kPartItems) {
        for (std::size_t stage = 0; stage < item.stages.size(); stage++) {
            const auto &parts = item.stages[stage];
            if (std::find(parts.begin(), parts.end(), part) != parts.end()) {
                std::string name = item.name;
                if (item.stages.size() > 1) {
                    name += " " + std::to_string(stage + 1);
                }
                toast("Locked: " + name);
                return;
            }
        }
    }
}

void Ap_GtMenuFrame(void)
{
    if (g_HeldShown || !g_Active || g_Received.empty()) {
        return;
    }
    g_HeldShown = true;
    std::vector<std::string> lines = {""};
    int count = 0;
    for (int id = 0; id < static_cast<int>(std::size(kManufacturerNames)); id++) {
        if (!g_Manufacturers[id] || kManufacturerNames[id] == nullptr) {
            continue;
        }
        count++;
        std::string name = kManufacturerNames[id];
        if (lines.back().size() + name.size() + 2 > kToastWidth) {
            lines.push_back("");
        }
        lines.back() += (lines.back().empty() ? "" : ", ") + name;
    }
    if (lines.size() >= kMaxToasts) {
        toast(std::to_string(count) + " manufacturers unlocked");
        return;
    }
    toast("Manufacturers you can use:");
    for (const std::string &line : lines) {
        toast(line);
    }
}

// Sent whether the slot has finish checks or not: the server ignores the IDs it doesn't have.
void Ap_RaceFinished(int race)
{
    if (g_RaceEvents[race] >= 0) {
        location(kLocationFinished + g_RaceEvents[race]);
    }
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
    if (tier < 0 || tier >= static_cast<long long>(std::size(kCredits))) {
        return 0;
    }
    toast("Added " + std::to_string(kCredits[tier]) + " Cr");
    return kCredits[tier];
}

// A championship's result is in its first race's slot, and its other races have none: it
// counts as won once all its races are, and finished once all are finished or won.
int Ap_EventChecked(int eventIndex)
{
    if (!g_Active) {
        return 0;
    }
    int races = 1;
    for (int first : kChampionships) {
        if (eventIndex > first && eventIndex < first + kChampionshipRaces) {
            return 0;
        }
        if (eventIndex == first) {
            races = kChampionshipRaces;
        }
    }
    int place = 1;
    for (int race = eventIndex; race < eventIndex + races; race++) {
        if (g_Done.count(kLocationEvents + race) != 0) {
            continue;
        }
        if (g_Done.count(kLocationFinished + race) == 0) {
            return 0;
        }
        place = 6;
    }
    return place;
}

int Ap_TestChecked(int license, int test)
{
    return g_Active && g_Done.count(kLocationTests + (kTiers - license) * 10 + test) != 0;
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
