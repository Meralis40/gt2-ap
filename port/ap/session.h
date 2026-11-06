// SPDX-License-Identifier: GPL-2.0-or-later
// The Archipelago protocol, on a thread of its own: networking never holds up the game.
// The game's thread hands it commands and drains its events.
#pragma once

#include <string>
#include <vector>

namespace ap {

struct Event {
    enum Kind {
        kConnected,     // text: seed name; numbers: the three trophy options
        kRefused,       // text: why. The session has stopped.
        kLost,          // text: why. The session keeps trying.
        kItems,         // index: first item's index; numbers: item IDs
        kChecked,       // numbers: location IDs the server has
        kMessage,       // text: for the player
    } kind;
    std::string text;
    int index = 0;
    std::vector<long long> numbers;
};

struct Login {
    std::string server, slot, password;
};

// Starts the session. Call once.
void Session_Start(const Login &login);
void Session_SendLocations(const std::vector<long long> &locations);
void Session_SendGoal();
void Session_RequestSync();
std::vector<Event> Session_TakeEvents();

}  // namespace ap
