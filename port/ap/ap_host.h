// SPDX-License-Identifier: GPL-2.0-or-later
// What the runtime asks of Archipelago: ap.h is for game code, which has no C++ library.
#pragma once

#include <string>
#include <vector>

// Each vblank: applies what the session has sent.
void Ap_Poll();
// --connect: connects without waiting for the page's button.
void Ap_SetLogin(const std::string &server, const std::string &slot, const std::string &password);
void Ap_DisablePage();
// With a replay: the session runs beside it from the start, with no page to shift its frames.
void Ap_ConnectNow();
// Text from the keyboard, for the field the page selected.
void Ap_PageText(const char *text);
void Ap_PageBackspace();
// Recent messages for the screen, oldest first, one line each.
std::string Ap_ToastLine(int index);
// One more of them, from the client itself.
void Ap_Toast(const std::string &text);

// What the tracker page shows. An entry has `have` of its `total` copies.
struct ApTrackerEntry {
    std::string name;
    int have, total;
};

struct ApTracker {
    std::vector<ApTrackerEntry> manufacturers, parts;
    bool trophyRule;  // whether trophies open the leagues
    int trophies, proNeeded, worldNeeded;
    long long credits;  // received in all
    int checks, checksTotal;
    int goalRaces, goalRacesTotal;
};

ApTracker Ap_Tracker();
