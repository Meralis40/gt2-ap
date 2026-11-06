// SPDX-License-Identifier: GPL-2.0-or-later
// What the runtime asks of Archipelago: ap.h is for game code, which has no C++ library.
#pragma once

#include <string>

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
