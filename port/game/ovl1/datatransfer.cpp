// OVL1 8001da08-8001e808 (datatransfer): Data transfer pages, GT1 record merging and
// conversion.
// The functions that don't match yet are the original assembly, with their C++
// under NON_MATCHING.
#include "gptr.h"
#include "common.h"
#include "gamestate.h"

// One language's strings of the second table, as a block for the struct copy.
struct MainStringsBlockB {
    char data[0x7a9];
};

// The GT1 save DataXfer_GetGt1Data returns, as far as it's read here.
struct Gt1Save {
    unsigned char pad0[0x2b5c];
    unsigned char licenseB[8]; // zero when not earned
    unsigned char licenseA[8];
};

extern "C" {
extern game_pointer<BaseMenu> (&g_CurrentMenu);
extern unsigned char (&D_80022D80)[];
extern char (&sB_LbFt)[];
extern MenuSprite (&g_DataTransfer_Items)[];
extern MenuSpriteStyle (&g_DataTransfer_ItemStyle);
extern SpriteDesc (&g_DataTransfer_Sprites)[];
extern MenuList (&g_DataTransfer_List);
extern game_pointer<BaseMenuPage> (&g_DataTransferMenu_Pages)[];
extern DataXferConfig (&g_OVL1_DataXferConfig);
extern unsigned int (&g_OVL1_EndFontInfo);
extern game_pointer<KanjiFont> (&g_OVL1_KanjiFont);
extern short (&g_CardSlotStatus)[];
extern BaseMenuPage (&g_Page_DataTransferMenu);
extern BaseMenuPage (&g_Page_DataTransferBuyCar);
void PushPage(BaseMenu *menu, BaseMenuPage *page);
int DataTransferMenu_ItemCallback(MenuListEventId event, MenuListEvent *ev, int index);
void Mix_MergeCourseRecords(void);
void Mix_MergeLicenseTimes(void);
void Mix_MergeMachineTest0(void);
void Mix_MergeMachineTest1(void);
void Mix_MergeMachineTest2(void);
void Convert_ApplyGt1Licenses(Gt1Save *gt1);

// Inflates the second string table and copies the current language's strings to
// 0x801ef6b0.
unsigned char MainMenu_LoadStringsB(void)
{
    MainStringsBlockB strings[8];

    Gzip_Inflate(D_80022D80, strings);
    *(MainStringsBlockB *)sB_LbFt = strings[g_GameState.language];
}

// Root page of Data Transfer: stores back, waits 20 frames, starts music track 7.
void DataTransferRoot_Enter(int back)
{
    BaseMenuPage *pMVar1;

    pMVar1 = g_CurrentMenu->activePage;
    ((RootMenuPage *)pMVar1)->back = back;
    ((RootMenuPage *)pMVar1)->delay = 0x14;
    MainMenu_PlayMusic(7);
}

// After the delay: pushes the Data Transfer menu, or leaves the menu (4) on the way back.
int DataTransferRoot_Update(int arg)
{
    BaseMenu *menu = g_CurrentMenu;
    RootMenuPage *page = (RootMenuPage *)menu->activePage;
    int result = 0;

    if (page->delay > 0) {
        if (--page->delay == 0) {
            result = 4;
            if (page->back != 1) {
                PushPage(menu, &g_Page_DataTransferMenu);
                result = 1;
            }
        }
    }
    return result;
}

// Empty.
void DataTransferRoot_Draw(DrawLayer *layer)
{
}

// Unreferenced page: 20-frame delay, MainMenu_StopMusic.
void DataTransferUnusedWait_Enter(int back)
{
    ((RootMenuPage *)g_CurrentMenu->activePage)->delay = 0x14;
    MainMenu_StopMusic();
}

// After the delay: leaves the menu (4).
int DataTransferUnusedWait_Update(int arg)
{
    RootMenuPage *page = (RootMenuPage *)g_CurrentMenu->activePage;
    int result = 0;

    if (page->delay > 0) {
        page->delay--;
        result = (page->delay == 0) * 4;
    }
    return result;
}

// Empty.
void DataTransferUnusedWait_Draw(DrawLayer *layer)
{
}

// A row of the Data Transfer menu: one sprite, from g_DataTransfer_Sprites.
int DataTransferMenu_ItemCallback(MenuListEventId event, MenuListEvent *ev, int index)
{
    MenuSprite *item = &g_DataTransfer_Items[index];
    int x;
    int y;

    switch (event) {
    case MLE_INIT:
        MenuSprite_Init(item, &g_DataTransfer_ItemStyle);
        item->sprite = &g_DataTransfer_Sprites[index];
        break;
    case MLE_REVEAL:
        item->anim = 0;
        break;
    case MLE_CLOSE:
        item->anim = -13;
        break;
    case MLE_UPDATE:
        MenuSprite_Update(item);
        break;
    case MLE_DRAW:
        x = ev->x;
        y = ev->y;
        item->x = x;
        item->y = y;
        MenuSprite_Draw(item, ev->ot, 0);
        break;
    case MLE_LEAVE:
        break;
    case MLE_ENTER:
        item->anim = 12;
        break;
    case MLE_OPEN:
    case MLE_IS_ENABLED:
        break;
    }
    return 1;
}

// Delays the list's opening 20 frames and, on the way back, keeps the chosen row.
void DataTransferMenu_Enter(int back)
{
    RootMenuPage *page = (RootMenuPage *)g_CurrentMenu->activePage;
    MenuList *list;
    int row;

    page->delay = 0x14;
    list = &g_DataTransfer_List;
    MenuList_Init(list, DataTransferMenu_ItemCallback, 0);
    list->selected = 0;
    if (back) {
        row = page->back;
        list->selected = row;
    }
}

// Opens the list after the delay. A chosen row is kept in back and pushes its page.
int DataTransferMenu_Update(int arg)
{
    BaseMenu *menu = g_CurrentMenu;
    RootMenuPage *page = (RootMenuPage *)menu->activePage;
    int result = 0;
    PadState *pad = menu->padState;
    int choice;
    MenuList *list;

    if (arg == 0) {
        pad = 0;
    }
    choice = page->delay;
    if (choice > 0) {
        choice--;
        if (choice == 0) {
            MenuList_Open(&g_DataTransfer_List);
        }
    }
    page->delay = choice;
    list = &g_DataTransfer_List;
    choice = MenuList_Update(list, pad);
    switch (choice) {
    case -4:
        break;
    case -3:
        Sfx_PlayMenu(5);
        break;
    case -2:
        break;
    case -1:
        Sfx_PlayMenu(4);
        MenuList_Close(list);
        result = 2;
        break;
    default:
        Sfx_PlayMenu(3);
        page->back = choice;
        MenuList_Close(&g_DataTransfer_List);
        result = 1;
        PushPage(g_CurrentMenu, g_DataTransferMenu_Pages[choice]);
        break;
    }
    return result;
}

// Draws the list.
void DataTransferMenu_Draw(DrawLayer *layer)
{
    uint *ot = (uint *)layer->ot;

    MenuList_Draw(&g_DataTransfer_List, ot + 4);
}

// Starts a transfer of mode 0, trading cars.
void DataTransferCarList_Enter(int back)
{
    g_OVL1_DataXferConfig.heap = g_OVL1_EndFontInfo;
    g_OVL1_DataXferConfig.font = g_OVL1_KanjiFont;
    g_OVL1_DataXferConfig.cardSlots = g_CardSlotStatus;
    DataXfer_Setup(&g_OVL1_DataXferConfig);
    DataXfer_Start(0);
}

// When the transfer ends, pushes the page that picks the car to buy.
int DataTransferCarList_Update(int arg)
{
    int result = 0;
    PadState *pad = g_CurrentMenu->padState;

    if (arg == 0) {
        pad = 0;
    }
    switch (DataXfer_Update(pad)) {
    case 2:
        Sfx_PlayMenu(3);
        PushPage(g_CurrentMenu, &g_Page_DataTransferBuyCar);
        result = 1;
        break;
    case 0:
        break;
    case 1:
        Sfx_PlayMenu(4);
        result = 2;
        break;
    }
    return result;
}

// DataXfer_Draw.
void DataTransferCarList_Draw(DrawLayer *layer)
{
    DataXfer_Draw((unsigned int *)rt::from_guest(((int)layer->ot + 0x10)));
}

// Takes each of the other save's course records that beats this save's, or fills an
// empty one.
void Mix_MergeCourseRecords(void)
{
    CourseRecord *src = (CourseRecord *)DataXfer_GetCourseRecords();
    int i;

    for (i = 0; i < 0x80; i++) {
        unsigned int time = src->best.time;

        if (~time != 0) {
            GameState *state = &g_GameState;
            unsigned int old;
            CourseRecord *dst = i + state->courseRecords;

            old = dst->best.time;
            if (~old != 0) {
                if (time < old) {
                    *dst = *src;
                }
            } else {
                *dst = *src;
            }
        }
        src++;
    }
}

// Where the time would rank in the test's best five: -1 if it's empty, doesn't place,
// or is already there under the same name. Equal times compare every field.
int Mix_FindTimeRank(TimeRecord *time, char *name, LicenseTest *test)
{
    int i;
    int j;
    int len;
    unsigned char *mine;
    unsigned char *theirs;

    if (~time->time == 0) {
        return -1;
    }
    for (i = 0; i < 5; i++) {
        TimeRecord *best = &test->bestTimes[i];
        unsigned int t = best->time;

        if (~t == 0) {
            return i;
        }
        if (time->time < t) {
            return i;
        }
        if (t == time->time) {
            theirs = (unsigned char *)best;
            mine = (unsigned char *)time;
            for (j = 0; j < 0x14; j++) {
                if (*theirs++ != *mine++) {
                    goto next;
                }
            }
            theirs = (unsigned char *)test->bestNames[i];
            mine = (unsigned char *)name;
            len = strlen((char *)theirs);
            if (len == strlen((char *)mine)) {
                for (j = 0; j <= len; j++) {
                    if (*theirs++ != *mine++) {
                        goto next;
                    }
                }
                return -1;
            }
        }
    next:;
    }
    return -1;
}

// Adds the other save's best times to each license test.
void Mix_MergeLicenseTimes(void)
{
    LicenseTest *src = (LicenseTest *)DataXfer_GetLicenseTests();
    int i;

    for (i = 0; i < 6; i++) {
        GameState *state = &g_GameState;
        int j;

        for (j = 0; j < 10; j++) {
            LicenseTest *dst = j + *(i + state->licenses);
            int k;

            for (k = 0; k < 5; k++) {
                TimeRecord *time = &src->bestTimes[k];
                char *name = src->bestNames[k];

                if (Mix_FindTimeRank(time, name, dst) >= 0) {
                    LicenseTest_InsertTime(dst, time, name);
                }
            }
            src++;
        }
    }
}

// Adds the other save's results to machine test 0.
void Mix_MergeMachineTest0(void)
{
    MachineTestRecords *src = (MachineTestRecords *)DataXfer_GetMachineTest0();
    GameState *state = &g_GameState;
    MachineTestRecords *records = &state->machineTests[0];
    int i;

    for (i = 0; i < src->count; i++) {
        MachineTestEntry *entry = &src->entries[i];

        MachineTest_InsertRecord(records, entry, 0);
    }
}

// Adds the other save's results to machine test 1.
void Mix_MergeMachineTest1(void)
{
    MachineTestRecords *src = (MachineTestRecords *)DataXfer_GetMachineTest1();
    GameState *state = &g_GameState;
    MachineTestRecords *records = &state->machineTests[1];
    int i;

    for (i = 0; i < src->count; i++) {
        MachineTestEntry *entry = &src->entries[i];

        MachineTest_InsertRecord(records, entry, 0);
    }
}

// Adds the other save's results to machine test 2.
void Mix_MergeMachineTest2(void)
{
    MachineTestRecords *src = (MachineTestRecords *)DataXfer_GetMachineTest2();
    GameState *state = &g_GameState;
    MachineTestRecords *records = &state->machineTests[2];
    int i;

    for (i = 0; i < src->count; i++) {
        MachineTestEntry *entry = &src->entries[i];

        MachineTest_InsertRecord(records, entry, 1);
    }
}

// Starts a transfer of mode 1, mixing records.
void DataTransferMix_Enter(int back)
{
    g_OVL1_DataXferConfig.heap = g_OVL1_EndFontInfo;
    g_OVL1_DataXferConfig.font = g_OVL1_KanjiFont;
    g_OVL1_DataXferConfig.cardSlots = g_CardSlotStatus;
    DataXfer_Setup(&g_OVL1_DataXferConfig);
    DataXfer_Start(1);
}

// When the transfer ends, merges the other save's records into this one.
int DataTransferMix_Update(int arg)
{
    int result = 0;
    PadState *pad = g_CurrentMenu->padState;

    if (arg == 0) {
        pad = 0;
    }
    switch (DataXfer_Update(pad)) {
    case 2:
        Sfx_PlayMenu(3);
        Mix_MergeCourseRecords();
        Mix_MergeLicenseTimes();
        Mix_MergeMachineTest0();
        Mix_MergeMachineTest1();
        Mix_MergeMachineTest2();
        break;
    case 0:
        break;
    case 1:
        Sfx_PlayMenu(4);
        result = 2;
        break;
    }
    return result;
}

// DataXfer_Draw.
void DataTransferMix_Draw(DrawLayer *layer)
{
    DataXfer_Draw((unsigned int *)rt::from_guest(((int)layer->ot + 0x10)));
}

// With all eight flags of a GT1 license set, every test of the matching license here
// that has no medal gets a kids prize: first license 5, then, if it was earned too,
// license 4.
void Convert_ApplyGt1Licenses(Gt1Save *gt1)
{
    int all = 1;
    int i;
    int j;

    for (i = 0; i < 8; i++) {
        if (gt1->licenseB[i] == 0) {
            all = 0;
        }
    }
    if (all) {
        for (j = 0; j < 10; j++) {
            LicenseTest *test = &g_GameState.licenses[5][j];

            if (*(signed char *)&test->medal == 0) {
                test->medal = MEDAL_KIDS;
            }
        }
        all = 1;
        for (i = 0; i < 8; i++) {
            if (gt1->licenseA[i] == 0) {
                all = 0;
            }
        }
        if (all) {
            int k;

            for (k = 0; k < 10; k++) {
                LicenseTest *test = &g_GameState.licenses[4][k];

                if (*(signed char *)&test->medal == 0) {
                    test->medal = MEDAL_KIDS;
                }
            }
        }
    }
}

// Starts a transfer of mode 2, converting a GT1 save.
void DataTransferConvert_Enter(int back)
{
    g_OVL1_DataXferConfig.heap = g_OVL1_EndFontInfo;
    g_OVL1_DataXferConfig.font = g_OVL1_KanjiFont;
    g_OVL1_DataXferConfig.cardSlots = g_CardSlotStatus;
    DataXfer_Setup(&g_OVL1_DataXferConfig);
    DataXfer_Start(2);
}

// When the transfer ends, grants what the GT1 save's licenses earn here.
int DataTransferConvert_Update(int arg)
{
    int result = 0;
    PadState *pad = g_CurrentMenu->padState;

    if (arg == 0) {
        pad = 0;
    }
    switch (DataXfer_Update(pad)) {
    case 2:
        Sfx_PlayMenu(3);
        Convert_ApplyGt1Licenses((Gt1Save *)DataXfer_GetGt1Data());
        break;
    case 0:
        break;
    case 1:
        Sfx_PlayMenu(4);
        result = 2;
        break;
    }
    return result;
}

// DataXfer_Draw.
void DataTransferConvert_Draw(DrawLayer *layer)
{
    DataXfer_Draw((unsigned int *)rt::from_guest(((int)layer->ot + 0x10)));
}
}
