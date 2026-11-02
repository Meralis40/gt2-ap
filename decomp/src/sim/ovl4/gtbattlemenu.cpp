// OVL4 8001508c-80015404 (gtbattlemenu): GTBattleMenu, which picks the battle
// opponents, and the battle opponent pool. The class's vtable and type name
// (.rodata), __tf function and typeinfo (.bss, which build.py's CXX_NAMES places)
// are emitted here.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "gtmenu.h"

extern "C" {
void PacketBuffer_Init(PacketBuffer *buf, void *base, unsigned int bytes);
void ViewDrawContext_Init(ViewDrawContext *ctx, void *ot, unsigned int bytes);
void ViewDrawContext_Reset(ViewDrawContext *ctx);
void ViewDrawContext_AddLayer(ViewDrawContext *ctx, DrawLayer *layer);
void BattleTable_Init(BattleCarTable *table);
int BattleTable_LoadNext(BattleCarTable *table);
void BattleTable_Sort(BattleCarTable *table);
void BattleTable_PickOpponents(BattleCarTable *table, BattlePlayer *player, OpponentPool *pool, int size,
                               int below);
void CardProgress_Reset(CardProgressBar *bar);
void CardProgress_Update(CardProgressBar *bar, int done);
void CardProgress_Draw(CardProgressBar *bar, void *ot);
RECT *Prim_AddTile(void *ot, unsigned int rgb);

extern GameState g_GameState;
extern unsigned char g_OVL4_OrderingTable[];
extern unsigned char g_Battle_PacketBuf[];
extern CardProgressBar g_Battle_Progress;
extern BattlePlayer g_Battle_Player;
extern OpponentPool g_Battle_Pool;
extern short g_Battle_PoolSize;
extern short g_Battle_PoolBelow;
}

// The work table, in g_RaceCarWorkAlt: the game takes it as g_GameState + 0xc6c0
// rounded down to a word each time.
inline BattleCarTable *Battle_Table()
{
    GameState *state = &g_GameState;
    return (BattleCarTable *)((unsigned int)((char *)state + 0xc6c0) & ~3);
}

extern "C" {
OpponentPool *Battle_GetOpponentPool(void)
{
    return &g_Battle_Pool;
}

// g_Battle_Player from the current car (power, weight) and the pool size
// parameters for GTBattleMenu.
#ifdef NON_MATCHING
// Scheduling and registers: the game stores g_Battle_PoolBelow between the
// flags98 load and the power store, and loads flags98 into $a0. Every order of
// the six statements (this one is among the closest), locals for the power,
// inline helpers for the car or the stores, and the four globals as one struct
// (which addresses them from one base) didn't give it. The car pointer has to be
// garage->cars + slot for the game's separate addiu 4.
void Battle_SetPlayer(short poolSize, short below)
{
    GameState *state = &g_GameState;
    Garage *garage = &state->garages[0];
    int slot = garage->currentCar;

    if (slot >= 0) {
        Car *car;

        g_Battle_PoolSize = poolSize;
        car = garage->cars + slot;
        g_Battle_Player.power = car->flags98 & 0x3fff;
        g_Battle_PoolBelow = below;
        g_Battle_Player.weight = car->stat94 & 0x1fff;
        g_Battle_Player.racingMod = 0;
    }
}
#else
INCLUDE_ASM("asm/sim/ovl4/nonmatchings/gtbattlemenu", Battle_SetPlayer);
#endif
}

GTBattleMenu::GTBattleMenu()
{
    noDrawSync = 1;
}

GTBattleMenu::~GTBattleMenu()
{
}

// Packets in g_Battle_PacketBuf, one layer, the table and the progress bar (its
// total is the car count).
void GTBattleMenu::Enter()
{
    BattleCarTable *table;

    psxViewLoop::Enter();
    SetVideoMode(0x64);
    PacketBuffer_Init(&packets, g_Battle_PacketBuf, 0x8c00);
    ViewDrawContext_Init(&draw, g_OVL4_OrderingTable, 0x2340);
    layer.priority = 0;
    layer.otSize = 0x10;
    ViewDrawContext_Reset(&draw);
    ViewDrawContext_AddLayer(&draw, &layer);
    table = Battle_Table();
    BattleTable_Init(table);
    g_Battle_Progress.total = table->count;
    CardProgress_Reset(&g_Battle_Progress);
    state = 0;
}

// State 0: BattleTable_LoadNext until done (progress bar). 1: BattleTable_Sort.
// 2: BattleTable_PickOpponents into g_Battle_Pool, returns 0 (leave).
int GTBattleMenu::Update()
{
    psxViewLoop::Update();
    switch (state) {
    case 0: {
        BattleCarTable *table = Battle_Table();

        if (BattleTable_LoadNext(table) != 0) {
            state++;
        }
        CardProgress_Update(&g_Battle_Progress, table->count - table->loaded);
        return 1;
    }
    case 1:
        BattleTable_Sort(Battle_Table());
        state++;
        break;
    case 2:
        BattleTable_PickOpponents(Battle_Table(), &g_Battle_Player, &g_Battle_Pool, g_Battle_PoolSize,
                                  g_Battle_PoolBelow);
        return 0;
    }
    return 1;
}

// The progress bar over a black tile.
void GTBattleMenu::Draw()
{
    void *ot = layer.ot;
    RECT *r;

    psxViewLoop::Draw();
    CardProgress_Draw(&g_Battle_Progress, ot);
    r = Prim_AddTile(ot, 0);
    r->w = 0x200;
    r->x = 0;
    r->y = 0;
    r->h = 0x1e0;
}
