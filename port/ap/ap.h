// SPDX-License-Identifier: GPL-2.0-or-later
// What the game code asks of Archipelago. Plain C names: game sources include this.
#ifndef AP_H
#define AP_H

#ifdef __cplusplus
extern "C" {
#endif

// Whether this run is an Archipelago session. Off, every rule stays the game's own.
int Ap_Active(void);

int Ap_Licenses(void);                    // Progressive Licenses held
int Ap_HasManufacturer(int manufacturer);
int Ap_HasPart(int part);
int Ap_LeagueTrophies(void);              // whether trophies replace the league rules
int Ap_ProLeagueOpen(void);
int Ap_WorldLeagueOpen(void);

// Say on screen why something was refused: nothing in the game does.
void Ap_SayManufacturerLocked(int manufacturer);
void Ap_SayPartLocked(int part);
// Each frame of the GT menus: the first one lists the manufacturers held.
void Ap_GtMenuFrame(void);

// Remembers the event of each race in a championship, then a finish (any place) or a win of
// race index.
void Ap_SetRaceEvent(int race, int eventIndex);
void Ap_RaceFinished(int race);
void Ap_RaceWon(int race);
void Ap_LicenseTestPassed(int license, int test);
void Ap_LicenseEarned(int license);

// The credits of the next item not yet applied to this save, whose count is at *applied:
// 0 for an item that isn't credits, -1 when there's none left.
int Ap_NextCredits(unsigned short *applied);

// The connection page. It needs a keyboard, so test runs and recordings go without it.
int Ap_PageWanted(void);
void Ap_PageOpen(void);
void Ap_PageClose(void);
void Ap_PageSelect(int field);            // the field text input goes to, -1 for none
void Ap_PageConnect(void);
const char *Ap_PageField(int field);      // 0 server, 1 slot, 2 password
int Ap_PageState(void);                   // AP_PAGE_*
const char *Ap_PageStatus(void);

enum { AP_PAGE_IDLE, AP_PAGE_CONNECTING, AP_PAGE_CONNECTED, AP_PAGE_FAILED };

#ifdef __cplusplus
}
#endif

#endif
