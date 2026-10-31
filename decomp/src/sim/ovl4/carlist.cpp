// OVL4 80020198-80020c50 (carlist): Garage and used car lists.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "ovl4.h"

extern "C" {
extern MenuList g_GarageList_Menu;
int GTText_Draw(void *ot, char *str, int x, int y, unsigned int rgb);
void MenuList_Draw(MenuList *list, void *ot);
extern CarSpecStrings g_CarSpecStrings;
extern MenuList g_UsedCarList_Menu;
extern UsedCarEntry *g_UsedCarListCars;

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carlist", GarageList_DrawItem);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carlist", GarageList_Init);

// Sets the position of the garage car list widget (0x80052958 +0x10 / +0x12); list is
// unused.
void CarList_SetPosition(void *list, short x, short y)
{
    g_GarageList_Menu.x = x;
    g_GarageList_Menu.y = y;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carlist", GarageList_Open);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carlist", GarageList_Update);

// Same as UsedCarList_DrawOrEmpty for the list widget at 0x80052958.
void CarList_DrawOrEmpty(short *list, void *layer)
{
    if (*list < 1) {
        GTText_Draw(layer, g_CarSpecStrings.emptyList, g_GarageList_Menu.x + -0x98, g_GarageList_Menu.y + 0x40, 0xf0780a);
    }
    else {
        MenuList_Draw(&g_GarageList_Menu, layer);
    }
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carlist", UsedCarList_DrawItem);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carlist", UsedCarList_Init);

// Sets the position of the used / new car list widget (0x8005299c +0x10 / +0x12); list
// is unused.
void UsedCarList_SetPosition(void *list, short x, short y)
{
    g_UsedCarList_Menu.x = x;
    g_UsedCarList_Menu.y = y;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carlist", UsedCarList_Build);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carlist", UsedCarList_Update);

// Draws the used car list, or g_CarSpecStrings.emptyList when it has no cars.
void UsedCarList_DrawOrEmpty(short *list, void *layer)
{
    if (*list < 1) {
        GTText_Draw(layer, g_CarSpecStrings.emptyList, g_UsedCarList_Menu.x + -0xbc, g_UsedCarList_Menu.y + 0x40, 0xf0780a);
    }
    else {
        MenuList_Draw(&g_UsedCarList_Menu, layer);
    }
}

unsigned int UsedCarList_GetSelectedCarId(void)
{
    return g_UsedCarListCars[g_UsedCarList_Menu.selected].carId;
}

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carlist", UsedCarList_GetSelectedColor);

INCLUDE_ASM("asm/sim/ovl4/nonmatchings/carlist", UsedCarList_GetSelectedPrice);
}
