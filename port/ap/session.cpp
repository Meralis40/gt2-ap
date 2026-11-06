// SPDX-License-Identifier: GPL-2.0-or-later
#include "session.h"

#include "ws.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <map>
#include <mutex>
#include <random>
#include <thread>

namespace ap {
namespace {

using json = nlohmann::json;

constexpr const char *kGame = "Gran Turismo 2";
constexpr int kItemsAll = 7;  // from other worlds, our own, and the starting inventory
constexpr int kStatusGoal = 30;
constexpr int kRetrySeconds = 5;
constexpr int kReceiveMs = 50;

struct NameTables {
    std::map<std::string, std::map<long long, std::string>> items, locations;  // by game
    std::map<int, std::string> playerNames, playerGames;
    int ownSlot = 0;
};

std::mutex g_Mutex;
std::vector<Event> g_Events;
std::vector<std::string> g_Outgoing;

void post(Event event)
{
    std::lock_guard<std::mutex> lock(g_Mutex);
    g_Events.push_back(std::move(event));
}

void queue(const json &command)
{
    json packet = json::array({command});
    std::lock_guard<std::mutex> lock(g_Mutex);
    g_Outgoing.push_back(packet.dump());
}

std::string randomId()
{
    static const char kDigits[] = "0123456789abcdef";
    std::random_device random;
    std::string id;

    for (int i = 0; i < 32; i++) {
        id += kDigits[random() & 15];
    }
    return id;
}

std::string lookup(const std::map<long long, std::string> &names, long long id, const char *fallback)
{
    auto found = names.find(id);
    return found != names.end() ? found->second : fallback;
}

// A PrintJSON's parts as text.
std::string flatten(const json &parts, const NameTables &names)
{
    std::string out;

    for (const json &part : parts) {
        std::string type = part.value("type", "text");
        std::string text = part.value("text", "");
        int player = part.value("player", 0);
        auto game = names.playerGames.find(player);
        if (type == "player_id") {
            auto name = names.playerNames.find(std::atoi(text.c_str()));
            out += name != names.playerNames.end() ? name->second : text;
        } else if (type == "item_id" && game != names.playerGames.end()) {
            auto table = names.items.find(game->second);
            out += table != names.items.end() ? lookup(table->second, std::atoll(text.c_str()), "an item") : "an item";
        } else if (type == "location_id" && game != names.playerGames.end()) {
            auto table = names.locations.find(game->second);
            out += table != names.locations.end() ? lookup(table->second, std::atoll(text.c_str()), "a location")
                                                  : "a location";
        } else {
            out += text;
        }
    }
    return out;
}

void onConnected(const json &packet, NameTables &names, const std::string &seed)
{
    names.ownSlot = packet.value("slot", 0);
    if (packet.contains("slot_info")) {
        for (const auto &[id, info] : packet["slot_info"].items()) {
            names.playerGames[std::atoi(id.c_str())] = info.value("game", "");
        }
    }
    for (const json &player : packet.value("players", json::array())) {
        names.playerNames[player.value("slot", 0)] = player.value("alias", player.value("name", ""));
    }
    const json options = packet.value("slot_data", json::object()).value("options", json::object());
    Event event{Event::kConnected};
    event.text = seed;
    event.numbers = {options.value("league_trophies", 0), options.value("pro_league_trophies", 0),
                     options.value("world_league_trophies", 0)};
    post(std::move(event));
    Event checked{Event::kChecked};
    for (const json &id : packet.value("checked_locations", json::array())) {
        checked.numbers.push_back(id.get<long long>());
    }
    post(std::move(checked));
}

void onPrint(const json &packet, const NameTables &names)
{
    // Only what concerns this player: the server tells everyone about everything.
    if (packet.value("type", "") == "ItemSend") {
        int receiving = packet.value("receiving", 0);
        int sender = packet.contains("item") ? packet["item"].value("player", 0) : 0;
        if (receiving != names.ownSlot && sender != names.ownSlot) {
            return;
        }
    } else if (packet.value("type", "") != "Goal" && packet.value("type", "") != "Countdown") {
        return;
    }
    Event event{Event::kMessage};
    event.text = flatten(packet.value("data", json::array()), names);
    post(std::move(event));
}

// Returns false when the session is over for good.
bool handle(const json &packet, const Login &login, NameTables &names, std::string &seed, const std::string &uuid)
{
    std::string cmd = packet.value("cmd", "");

    if (cmd == "RoomInfo") {
        seed = packet.value("seed_name", "");
        queue({{"cmd", "GetDataPackage"}, {"games", packet.value("games", json::array())}});
        json version = {{"major", 0}, {"minor", 6}, {"build", 8}, {"class", "Version"}};
        queue({{"cmd", "Connect"},
               {"password", login.password},
               {"game", kGame},
               {"name", login.slot},
               {"uuid", uuid},
               {"version", version},
               {"items_handling", kItemsAll},
               {"tags", json::array()},
               {"slot_data", true}});
    } else if (cmd == "DataPackage") {
        for (const auto &[game, data] : packet["data"]["games"].items()) {
            const json itemIds = data.value("item_name_to_id", json::object());
            const json locationIds = data.value("location_name_to_id", json::object());
            for (const auto &[name, id] : itemIds.items()) {
                names.items[game][id.get<long long>()] = name;
            }
            for (const auto &[name, id] : locationIds.items()) {
                names.locations[game][id.get<long long>()] = name;
            }
        }
    } else if (cmd == "Connected") {
        onConnected(packet, names, seed);
    } else if (cmd == "ConnectionRefused") {
        std::string why = "refused:";
        for (const json &error : packet.value("errors", json::array())) {
            why += " " + error.get<std::string>();
        }
        Event event{Event::kRefused};
        event.text = why;
        post(std::move(event));
        return false;
    } else if (cmd == "ReceivedItems") {
        Event event{Event::kItems};
        event.index = packet.value("index", 0);
        for (const json &item : packet.value("items", json::array())) {
            event.numbers.push_back(item.value("item", 0LL));
        }
        post(std::move(event));
    } else if (cmd == "RoomUpdate" && packet.contains("checked_locations")) {
        Event event{Event::kChecked};
        for (const json &id : packet["checked_locations"]) {
            event.numbers.push_back(id.get<long long>());
        }
        post(std::move(event));
    } else if (cmd == "PrintJSON") {
        onPrint(packet, names);
    }
    return true;
}

void run(Login login)
{
    WebSocket socket;
    NameTables names;
    std::string seed;
    std::string uuid = randomId();
    bool everConnected = false;

    for (;;) {
        std::string error;
        if (!socket.open(login.server, error)) {
            Event event{everConnected ? Event::kLost : Event::kRefused};
            event.text = error;
            post(std::move(event));
            if (!everConnected) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::seconds(kRetrySeconds));
            continue;
        }
        everConnected = true;
        {
            // Anything queued for the last connection goes out with a fresh Connected.
            std::lock_guard<std::mutex> lock(g_Mutex);
            g_Outgoing.clear();
        }
        for (;;) {
            std::vector<std::string> outgoing;
            {
                std::lock_guard<std::mutex> lock(g_Mutex);
                outgoing.swap(g_Outgoing);
            }
            bool alive = true;
            for (const std::string &text : outgoing) {
                alive = alive && socket.sendText(text);
            }
            std::string message;
            int got = alive ? socket.receive(message, kReceiveMs) : -1;
            if (got < 0) {
                break;
            }
            if (got == 0) {
                continue;
            }
            try {
                for (const json &packet : json::parse(message)) {
                    if (!handle(packet, login, names, seed, uuid)) {
                        socket.close();
                        return;
                    }
                }
            } catch (const std::exception &e) {
                Event event{Event::kMessage};
                event.text = std::string("bad message from the server: ") + e.what();
                post(std::move(event));
            }
        }
        socket.close();
        Event lost{Event::kLost};
        lost.text = "connection lost";
        post(std::move(lost));
        std::this_thread::sleep_for(std::chrono::seconds(kRetrySeconds));
    }
}

}  // namespace

void Session_Start(const Login &login)
{
    std::thread(run, login).detach();
}

void Session_SendLocations(const std::vector<long long> &locations)
{
    queue({{"cmd", "LocationChecks"}, {"locations", locations}});
}

void Session_SendGoal()
{
    queue({{"cmd", "StatusUpdate"}, {"status", kStatusGoal}});
}

void Session_RequestSync()
{
    queue({{"cmd", "Sync"}});
}

std::vector<Event> Session_TakeEvents()
{
    std::lock_guard<std::mutex> lock(g_Mutex);
    std::vector<Event> events;
    events.swap(g_Events);
    return events;
}

}  // namespace ap
