// OVL2 80010c84-80011a50 (arcademode): Arcade race setup, the arcade mode loop and its strings.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "gamestate.h"

extern "C" {

// The arcade menus' strings of one language.
struct ArcStrings {
    char data[0x314];
};

extern FlowMenuState g_Flow_IntroShown;
extern MenuPage g_ArcPage_Root;
extern ArcadeRaceSel g_ArcSel_Level;
extern ArcStrings g_OVL2_Strings;
extern unsigned char g_OVL2_StringsGz[];

INCLUDE_ASM("asm/sim/ovl2/nonmatchings/arcademode", ArcadeRace_Setup);

// The arcade mode: the menus, then what they chose (g_Flow_IntroShown.arcadeMenuExit):
// 0 back to the title, 1 the race (ArcadeRace_Setup with two random seeds and the
// loading screen, then OVL3 with the selection in g_RaceCarWorkAlt), 2 and 3 the
// movie (OVL5), 4 OVL1's next demo.
void ArcadeMode_Run(void)
{
    int r;

    ArcadeMenu_LoadStrings();
    {
        ArcadeMenu menu;

        ArcadeMenu_SetRootPage(&menu, &g_ArcPage_Root);
        do {
            r = menu.Execute();
        } while (r > 1 ? 1 : r < 0);
    }
    switch (g_Flow_IntroShown.arcadeMenuExit) {
    case 4:
        Overlay_StartAt(1, (void *)MainMenu_StartNextDemo, 0);
        break;
    case 0:
        g_Flow_IntroShown.homeOverlay = 0;
        g_Flow_IntroShown.gameMode = 0;
        Overlay_Start(1);
        break;
    case 2:
        g_Flow_IntroShown.homeOverlay = 0;
        g_Flow_IntroShown.gameMode = 0;
        Overlay_StartAt(5, (void *)Movie_ArcadeEntry, 1);
        break;
    case 3:
        g_Flow_IntroShown.homeOverlay = 0;
        g_Flow_IntroShown.gameMode = 0;
        Overlay_StartAt(5, (void *)Movie_ArcadeEntry, 0);
        break;
    case 1: {
        unsigned int seed = Irq_WaitVSyncs(0);
        unsigned int seed1 = Rand_Next(&seed);
        unsigned int seed2 = Rand_Next(&seed);

        ArcadeRace_Setup(seed1, seed2, &g_ArcSel_Level);
        {
            ArcadeRacingMenu loading;

            do {
                r = loading.Execute();
            } while (r > 1 ? 1 : r < 0);
        }
        {
            char *base = (char *)&g_GameState;
            int offset = 0xc6c0;

            g_Flow_IntroShown.homeOverlay = 1;
            g_Flow_IntroShown.gameMode = 1;
            *(ArcadeRaceSel *)((int)(base + offset) & ~3) = g_ArcSel_Level;
        }
        Overlay_Start(3);
        Overlay_StartAt(0, (void *)Race_Entry, 0);
        break;
    }
    }
}

// Inflates the strings of every language on the stack and keeps the current language's.
void ArcadeMenu_LoadStrings(void)
{
    ArcStrings strings[8];

    Gzip_Inflate(g_OVL2_StringsGz, strings);
    g_OVL2_Strings = strings[g_GameState.language];
}
}
