// OVL1 80011178-800117b4 (mainmenu): The main menu's assets, its run loop and the demos.
#include "copies.h"
#include "common.h"
#include "gamestate.h"
#include "ap.h"

extern "C" {
extern FlowMenuState g_Flow_IntroShown;
extern void *g_EventDataBuf;
extern unsigned char g_OVL1_FontInfo[];
extern CarInfoA g_OVL1_storageCarInfoA;
extern unsigned char g_OVL1_DataBuf[];
extern unsigned char g_OVL1_ReplayBuf[];
extern short g_MainMenu_TitleTims[];
extern VolBootId g_MainMenu_TitlePictures[];
extern BaseMenuPage g_Page_ApConnect;
extern BaseMenuPage g_Page_AutoLoad;
extern BaseMenuPage g_Page_MainMenu;
extern BaseMenuPage g_Page_ReplayTheater;
extern BaseMenuPage g_Page_Options;
extern BaseMenuPage g_Page_SaveGame;
extern BaseMenuPage g_Page_LoadGame;
extern BaseMenuPage g_Page_DataTransfer;

// Loads arc_key_config.tim and arc_font.tim, and the car info.
void MainMenu_LoadCommonAssets(void)
{
    Tim_LoadFile(g_OVL1_FontInfo, VOLID_ARCADE_ARC_KEY_CONFIG_TIM);
    Tim_UploadToPage_OVL2(g_OVL1_FontInfo, 0x1d);
    Tim_LoadFile(g_OVL1_FontInfo, VOLID_ARCADE_ARC_FONT_TIM);
    Tim_UploadToPage_OVL2(g_OVL1_FontInfo, 0x1e);
    LoadCarInfoA(&g_OVL1_storageCarInfoA, 0xc800);
}

// Loads the title textures of the current language, and the title picture's CLUT
// and image to (0x180, 0x1ff) and (0x180, 0).
void MainMenu_LoadTitleAssets(void)
{
    unsigned int lang = g_GameState.language;
    int *block;
    int *image;
    RECT rect;

    Tim_LoadFile(g_OVL1_FontInfo, (VolBootId)g_MainMenu_TitleTims[lang]);
    Tim_UploadToPage_OVL2(g_OVL1_FontInfo, 0xe);
    VolId_ReadFileAsync(VOLID_ARCADE_TITLE_ITEM_TIM, g_OVL1_FontInfo);
    Tim_UploadToPage_OVL2(g_OVL1_FontInfo, 0xc);
    VolId_ReadFileAsync(g_MainMenu_TitlePictures[lang], g_OVL1_FontInfo);
    block = (int *)(g_OVL1_FontInfo + 8);
    rect.x = 0x180;
    rect.y = 0x1ff;
    rect.w = ((short *)block)[4];
    rect.h = ((short *)block)[5];
    Gpu_LoadImageBlock(block, &rect);
    rect.x = 0x180;
    rect.y = 0;
    image = (int *)((int)block + *(int *)(g_OVL1_FontInfo + 8));
    rect.w = ((short *)image)[4];
    rect.h = ((short *)image)[5];
    Gpu_LoadImageBlock(image, &rect);
}

// Loads l_sel.tim's CLUT and image to (0x180, 0x1ff) and (0x180, 0), and langsel.tim
// to texture page 0x1c. Nothing calls it.
void FUN_OVL1__800112d0(void)
{
    int *block;
    int *image;
    RECT rect;

    VolId_ReadFileAsync(VOLID_ARCADE_L_SEL_TIM, g_OVL1_FontInfo);
    block = (int *)(g_OVL1_FontInfo + 8);
    rect.x = 0x180;
    rect.y = 0x1ff;
    rect.w = ((short *)block)[4];
    rect.h = ((short *)block)[5];
    Gpu_LoadImageBlock(block, &rect);
    rect.x = 0x180;
    rect.y = 0;
    image = (int *)((int)block + *(int *)(g_OVL1_FontInfo + 8));
    rect.w = ((short *)image)[4];
    rect.h = ((short *)image)[5];
    Gpu_LoadImageBlock(image, &rect);
    VolId_ReadFileAsync(VOLID_ARCADE_LANGSEL_TIM, g_OVL1_FontInfo);
    Tim_UploadToPage_OVL2(g_OVL1_FontInfo, 0x1c);
}

// OVL1's entry: the title and the main menu. On the first boot, the save's auto load
// (g_Page_AutoLoad). Back from a replay (g_Flow_GameMode 2) it re-enters the Replay
// Theater, back from a demo race it starts the next one. Then the main menu, and the
// menu of its choice (g_Flow_MainMenuChoice): GT mode (OVL4), the Replay Theater
// (a chosen replay is played in OVL0), the options, the save and load pages, the
// data transfer, or, after 15 s idle, the next demo replay.
void MainMenu_Run(void)
{
    int r;

    MainMenu_LoadCommonAssets();
    if (!g_Flow_IntroShown.introShown) {
        MainMenu_LoadStrings();
        g_Flow_IntroShown.introShown = 1;
        if (Ap_PageWanted()) {
            BaseMenu menu;

            BaseMenu__SetRootPage(&menu, &g_Page_ApConnect);
            do {
                r = menu.Execute();
            } while (r > 1 ? 1 : r < 0);
        }
        {
            BaseMenu menu;

            BaseMenu__SetRootPage(&menu, &g_Page_AutoLoad);
            do {
                r = menu.Execute();
            } while (r > 1 ? 1 : r < 0);
        }
    }
    MainMenu_LoadStrings();
    {
        GameState *gs = &g_GameState;
        PlayerStats *stats;

        Options_ApplyNeGconCalib(gs);
        stats = &gs->stats;
        MainMenu_LoadTitleAssets();
        stats->unk44 = 0;
        stats->unk46 = 0;
    }
    switch (g_Flow_IntroShown.gameMode) {
    case 0:
        break;
    case 2:
        goto replayTheater;
    case 3:
    default:
        Overlay_StartAt(1, MainMenu_StartNextDemo, 0);
    }
    for (;;) {
        {
            BaseMenu menu;

            BaseMenu__SetRootPage(&menu, &g_Page_MainMenu);
            do {
                r = menu.Execute();
            } while (r > 1 ? 1 : r < 0);
        }
        EnableDisplay(1);
        switch (g_Flow_IntroShown.mainMenuChoice) {
        case 0:
            g_Flow_IntroShown.homeOverlay = 4;
            g_Flow_IntroShown.gameMode = 3;
            Overlay_Start(4);
            goto saveGame;
        case 1:
            g_Flow_IntroShown.homeOverlay = 2;
            g_Flow_IntroShown.gameMode = 2;
            goto replayTheater;
        default:
        case 3:
        saveGame: {
            BaseMenu menu;

            BaseMenu__SetRootPage(&menu, &g_Page_SaveGame);
            do {
                r = menu.Execute();
            } while (r > 1 ? 1 : r < 0);
            break;
        }
        case 4: {
            BaseMenu menu;

            BaseMenu__SetRootPage(&menu, &g_Page_LoadGame);
            do {
                r = menu.Execute();
            } while (r > 1 ? 1 : r < 0);
            break;
        }
        case 2: {
            BaseMenu menu;

            BaseMenu__SetRootPage(&menu, &g_Page_Options);
            do {
                r = menu.Execute();
            } while (r > 1 ? 1 : r < 0);
            Options_ApplyNeGconCalib(&g_GameState);
            break;
        }
        case 5: {
            BaseMenu menu;

            BaseMenu__SetRootPage(&menu, &g_Page_DataTransfer);
            do {
                r = menu.Execute();
            } while (r > 1 ? 1 : r < 0);
            break;
        }
        case 6: {
            FlowMenuState *flow = &g_Flow_IntroShown;
            int index = flow->demoReplayIndex;
            int count;

            flow->homeOverlay = 0;
            flow->gameMode = 0;
            count = DemoReplay_Count(g_OVL1_FontInfo);
            DemoReplay_Load(g_OVL1_FontInfo, g_OVL1_ReplayBuf, index);
            index++;
            if (index >= count) {
                index = 0;
            }
            flow->demoReplayIndex = index;
            goto startReplay;
        }
        replayTheater: {
            BaseMenu menu;

            BaseMenu__SetRootPage(&menu, &g_Page_ReplayTheater);
            do {
                r = menu.Execute();
            } while (r > 1 ? 1 : r < 0);
            BaseMenu *m = &menu;

            if (m->result) {
                goto startReplay;
            }
            g_Flow_IntroShown.homeOverlay = 0;
            g_Flow_IntroShown.gameMode = 0;
            break;
        }
        startReplay:
            Race_PrepareStart();
            Irq_WaitVSyncs(1);
            EnableDisplay(1);
            Irq_WaitVSyncs(2);
            Overlay_StartAt(0, Race_Entry, 1);
            EnableDisplay(1);
            break;
        }
    }
}

// The attract mode's demo race, entered through Overlay_StartAt(1, ...) by
// MainMenu_Run: loads the arcade data and demo race g_Flow_DemoReplayIndex (after
// advancing it), builds its grid and starts it in OVL0.
void MainMenu_StartNextDemo(void)
{
    FlowMenuState *flow = &g_Flow_IntroShown;

    flow->gameMode = 0;
    flow->demoReplayIndex++;
    g_EventDataBuf = GameData_LoadArcade(g_OVL1_DataBuf, 0x30000);
    DemoRace_LoadSetup(flow->demoReplayIndex);
    RaceSetup_InitEvent(1, 0, 0, 0, 0, 0, 0);
    Overlay_StartAt(0, Race_Entry, 1);
}
}
