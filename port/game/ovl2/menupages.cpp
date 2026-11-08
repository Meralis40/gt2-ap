// OVL2 80015670-80015834 (menupages): ArcadeMenu's BeginFrame, its hook and its
// page stack.
#include "common.h"

extern "C" {
extern game_pointer<KanjiFont> (&g_ArcKanjiFont);
}

// Runs the hook (step 0), uploads the kanji glyphs, re-applies the car views'
// changed colours, and uploads one of the two logos' pending images, alternating
// each frame.
void ArcadeMenu::BeginFrame()
{
    int i;

    psxViewLoop::BeginFrame();
    if (hookActive != 0) {
        hookActive = hook(hookArg, 1, 0);
    }
    KanjiFont_Upload(g_ArcKanjiFont);
    for (i = 0; i < 2; i++) {
        ArcCarView *view = carViews[i];
        if (view->colorDirty != 0) {
            ArcCarView_ApplyColor(view);
        }
    }

    for (i = 0; i < 2; i++) {
        if ((frameCount & 1) == i) {
            ArcLogo_UploadPending(&logos[i], logoArcReady);
        }
    }
}

// Installs the per-frame hook if none is active. Returns 1 if installed.
int ArcadeMenu_SetHook(ArcadeMenu *menu, unsigned char (*hook)(void *, int, int), void *arg)
{
    if (menu->hookActive != 0) {
        return 0;
    }
    menu->hookActive = 1;
    menu->hook = hook;
    menu->hookArg = arg;
    return 1;
}

// Calls the active hook with (arg, 0, 2) and removes it.
void ArcadeMenu_CancelHook(ArcadeMenu *menu)
{
    if (menu->hookActive != 0) {
        menu->hook(menu->hookArg, 0, 2);
        menu->hookActive = 0;
    }
}

// Pushes page; the current one becomes the leaving page.
void ArcadeMenu_PushPage(ArcadeMenu *menu, MenuPage *page)
{
    menu->leavingPage = menu->pageStack[menu->pageDepth];
    menu->pageDepth++;
    menu->pageStack[menu->pageDepth] = page;
}

// Pops back to the previous page, which it returns the depth of; -1 at the root.
int ArcadeMenu_PopPage(ArcadeMenu *menu)
{
    if (menu->pageDepth <= 0) {
        return -1;
    }
    menu->leavingPage = menu->pageStack[menu->pageDepth];
    return --menu->pageDepth;

}
