// OVL2 800228c0-8002344c (courseselect): Course selection page.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "common.h"
#include "syslib.h"

// Runs the bar's animation backwards.
static inline void WipeBar_Reverse(WipeBar *bar)
{
    bar->t = ~bar->duration;
}

extern "C" {
extern GameState g_GameState;
extern ArcadeRaceSel g_ArcSel_Level;
extern short g_Arc_CourseSet;
extern ArcCourse g_ArcCourses_Race[];
extern ArcCourse g_ArcCourses_RaceRev[];
extern ArcCourse g_ArcCourses_TimeTrial[];
extern ArcCourse g_ArcCourses_TimeTrialRev[];
extern ArcCourse g_ArcCourses_Rally[];
extern ArcCourse g_ArcCourses_2P[];
extern ArcCourse g_ArcCourses_2PRally[];
extern short g_Arc_CourseCount;
extern short D_800F3958;
extern short D_800F395A;
extern short D_800F395C;
extern short D_800F395E;
extern short D_800F3960;
extern short D_800F3962;
extern ArcGridPicker g_Arc_CourseCarousel;
extern ArcGridPickerTmpl D_80050164;
extern WipeBar D_80050174;
extern WipeBar D_80050190;
extern WipeBar D_800501AC;
extern short D_800501C8[];
extern short D_800501DC[];
extern short D_800F3918[];
extern short D_800F3938[];
extern ArcRecordPanel g_ArcCourseRecordPanel;
extern ArcRecordPanel g_ArcCourseInfoPanel;
extern MenuPage g_ArcPage_RaceLaunch;
extern Font D_80129820;
extern Font D_80123810;
extern unsigned char D_80052194[]; // the colours
extern unsigned char D_80052198[];
extern unsigned char D_8005219C[];
extern unsigned char D_800521A0[];
extern char D_80027438[];
extern char D_800F8731[];
extern char D_800F8623[];
extern short g_Arc_MovieId;
extern short g_Arc_MovieDelay;

// Starts the course preview movie after 12 frames.
void CourseSelect_QueueMovie(short movie)
{
    g_Arc_MovieId = movie;
    g_Arc_MovieDelay = 0xc;
}

// The current set's lists and their lengths. Rally and two-player sets have one list.
void ArcCourseList_GetCurrent(ArcCourse **forward, ArcCourse **reverse, int *count, int *reverseCount)
{
    ArcCourse *f;
    ArcCourse *r;

    switch (g_Arc_CourseSet) {
    case 0:
        f = g_ArcCourses_Race;
        *count = g_Arc_CourseCount;
        r = g_ArcCourses_RaceRev;
        *reverseCount = D_800F3958;
        break;
    case 1:
        f = g_ArcCourses_TimeTrial;
        *count = D_800F395A;
        r = g_ArcCourses_TimeTrialRev;
        *reverseCount = D_800F395C;
        break;
    case 2:
        r = g_ArcCourses_Rally;
        *count = D_800F395E;
        *reverseCount = D_800F395E;
        f = r;
        break;
    case 3:
        r = g_ArcCourses_2P;
        *count = D_800F3960;
        *reverseCount = D_800F3960;
        f = r;
        break;
    case 4:
        r = g_ArcCourses_2PRally;
        *count = D_800F3962;
        *reverseCount = D_800F3962;
        f = r;
        break;
    }
    *forward = f;
    *reverse = r;
}

// The reverse course when the row asks for it and it is unlocked.
ArcCourse *ArcCourseList_Pick(ArcCourse *forward, ArcCourse *reverse, int row, int index)
{
    ArcCourse *course = forward;

    if (row != 0) {
        course = reverse;
    }
    course += index;
    if (!course->unlocked) {
        course = &forward[index];
    }
    return course;
}

// The course number and name, marked when flag 4 is set, with the map under the current
// one.
void CourseSelect_DrawItem(ArcGridPickerDraw *ctx, int item)
{
    char line[0x80];
    char mark[8];
    ArcCourse *forward;
    ArcCourse *reverse;
    int count;
    int reverseCount;
    ArcadeMenu *menu = (ArcadeMenu *)ctx->arg;
    ArcCourseMap *map = &menu->courseMap;
    TextCtx &text = *menu->text;
    void *ot = ctx->ot;
    int x = ctx->x;
    int y = ctx->y;
    int brightness = ctx->brightness;
    int index;
    int row;
    ArcCourse *course;

    bzero(line, 0x80);
    mark[0] = 0x7f;
    mark[1] = 0;
    index = ctx->pos.item;
    row = ctx->pos.row;
    if (item >= 0) {
        Text_SetFont(&text, &D_80129820);
        text.ot = (uint *)ot;
        *(uint *)&text.clut = (*(uint *)&text.clut & 0xff9fffff) | 0x200000;
        Rgb_Lerp(D_80052194, D_80052198, brightness, 0x80);
        ArcCourseList_GetCurrent(&forward, &reverse, &count, &reverseCount);
        course = ArcCourseList_Pick(forward, reverse, row, index);
        sprintf(line, D_80027438, index + 1);
        if ((course->flags >> 4) & 1) {
            strcat(line, mark);
            Rgb_Lerp(D_80052194, D_8005219C, brightness, 0x80);
            *(uint *)&text.clut = (*(uint *)&text.clut & 0xff9fffff) | 0x400000;
        }
        strcat(line, course->label);
        text.rgb = brightness | brightness << 8 | brightness << 16 | 0x2000000;
        Text_DrawCentered(&text, (signed char *)line, x, y + 0xc, 1);
        if (item == index) {
            CourseMap_Draw(map, ot, x, y + 0x82, brightness);
        }
    }
}

// Opens the course's record and info panels.
void CourseSelect_SetCourse(unsigned int hash, void *info)
{
    int index = Course_FindByHash(hash);
    GameState *state = &g_GameState;

    CourseRecordPanel_Open(&g_ArcCourseRecordPanel, (char *)state + (index * 36 + 0x218));
    SlideAnim_Open(&g_ArcCourseInfoPanel, info);
}

// One column, or two when the set has reverse courses.
void CourseSelect_Enter(ArcadeMenu *menu, int back)
{
    int counts[2];
    ArcCourse *forward;
    ArcCourse *reverse;
    int columns = 1;
    ArcPage *page = (ArcPage *)menu->activePage;
    ArcCourseMap *map = &menu->courseMap;

    ArcCourseList_GetCurrent(&forward, &reverse, &counts[0], &counts[1]);
    page->delay = 0x18;
    page->choice = 0;
    switch (g_Arc_CourseSet) {
    case 2:
    case 3:
    case 4:
        break;
    default:
        if (counts[1] > 0) {
            columns = 2;
        }
        break;
    }
    D_80050164.draw = CourseSelect_DrawItem;
    D_80050164.drawArg = menu;
    counts[1] = counts[0];
    GridPicker_SetColumns(&g_Arc_CourseCarousel, columns, counts);
    GridPicker_Init(&g_Arc_CourseCarousel, &D_80050164, 0);
    CourseMap_Cancel(map, menu);
    CourseMap_Request(map, menu, forward->hash);
    D_80050174.t = -1;
    D_80050190.t = -1;
    D_800501AC.t = -1;
    CourseRecordPanel_Init(&g_ArcCourseRecordPanel);
    SlideAnim_Clear(&g_ArcCourseInfoPanel);
    GradientBar_Init(D_800F3918, D_800501C8);
    GradientBar_Init(D_800F3938, D_800501DC);
    g_Arc_MovieDelay = -1;
}

#ifdef NON_MATCHING
// The carousel, the record and info panels, and the preview movie. Returns 2 to go back,
// and 1 once the course is chosen and the launch page pushed.
int CourseSelect_Update(ArcadeMenu *menu, int active)
{
    ArcCourse *forward;
    ArcCourse *reverse;
    int count;
    int reverseCount;
    int result = 0;
    ArcPage *page = (ArcPage *)menu->activePage;
    ArcCourseMap &map = menu->courseMap;
    int choice;
    int index;

    ArcCourseList_GetCurrent(&forward, &reverse, &count, &reverseCount);
    if (page->delay > 0) {
        if (--page->delay == 0) {
            GridPicker_Open(&g_Arc_CourseCarousel);
            CourseSelect_SetCourse(forward->hash, forward->info);
            CourseSelect_QueueMovie(forward->info->movie);
            D_80050174.t = 0;
            D_80050190.t = 0;
            D_800501AC.t = 0;
            GradientBar_Start(D_800F3918);
            GradientBar_Start(D_800F3938);
        }
    }
    CourseInfoPanel_Update(&g_ArcCourseInfoPanel);
    WipeBar_Update(&D_80050174);
    WipeBar_Update(&D_80050190);
    WipeBar_Update(&D_800501AC);
    CourseRecordPanel_Update(&g_ArcCourseRecordPanel);
    GradientBar_Update(D_800F3918);
    GradientBar_Update(D_800F3938);
    if (page->choice == 0) {
        if (g_Arc_MovieDelay >= 0) {
            if (--g_Arc_MovieDelay == 0) {
                ArcadeMenu_PlayMovie(menu, g_Arc_MovieId);
            }
        }
    }
    choice = GridPicker_Update(&g_Arc_CourseCarousel, &menu->padState[0]);
    index = g_Arc_CourseCarousel.cur.item;
    switch (choice) {
    case -2:
        break;
    case -3: {
        ArcCourse *course;

        Sfx_PlayMenu(7);
        ArcadeMenu_StopMovie(menu);
        course = ArcCourseList_Pick(forward, reverse, g_Arc_CourseCarousel.cur.row, index);
        CourseMap_Request(&map, menu, course->hash);
        CourseSelect_QueueMovie(course->info->movie);
        CourseSelect_SetCourse(course->hash, course->info);
        return result;
    }
    case -1:
        Sfx_PlayMenu(4);
        page->choice = 1;
        ArcadeMenu_StopMovie(menu);
        g_Arc_MovieDelay = -1;
        CourseMap_Cancel(&map, menu);
        GridPicker_Close(&g_Arc_CourseCarousel);
        SlideAnim_Close(&g_ArcCourseInfoPanel);
        result = 2;
        WipeBar_Reverse(&D_80050174);
        WipeBar_Reverse(&D_80050190);
        WipeBar_Reverse(&D_800501AC);
        CourseRecordPanel_Close(&g_ArcCourseRecordPanel);
        GradientBar_Close(D_800F3918);
        GradientBar_Close(D_800F3938);
        return result;
    default:
        if (map.ready != 0) {
            ArcCourse *course;
            unsigned int hash;
            int stat;

            ArcadeBgm_Stop();
            page->choice = 1;
            ArcadeMenu_StopMovie(menu);
            CourseMap_Cancel(&map, menu);
            Sfx_PlayMenu(3);
            GridPicker_Close(&g_Arc_CourseCarousel);
            SlideAnim_Close(&g_ArcCourseInfoPanel);
            WipeBar_Reverse(&D_80050174);
            WipeBar_Reverse(&D_80050190);
            WipeBar_Reverse(&D_800501AC);
            CourseRecordPanel_Close(&g_ArcCourseRecordPanel);
            GradientBar_Close(D_800F3918);
            GradientBar_Close(D_800F3938);
            ArcadeMenu_PushPage(menu, &g_ArcPage_RaceLaunch);
            course = ArcCourseList_Pick(forward, reverse, g_Arc_CourseCarousel.cur.row, index);
            hash = course->hash;
            stat = course->stat;
            strcpy(g_ArcSel_Level.courseName, course->label);
            g_ArcSel_Level.courseHash = hash;
            g_ArcSel_Level.courseStat = stat;
            result = 1;
            g_ArcSel_Level.courseMap = map.buf2;
        } else {
            Sfx_PlayMenu(0);
        }
        break;
    }
    return result;
}
#else
INCLUDE_ASM("asm/sim/ovl2/nonmatchings/courseselect", CourseSelect_Update);
#endif

// The carousel and the movie, the three titled bars, the panels and the two gradient bars.
void CourseSelect_Draw(ArcadeMenu *menu, DrawLayer *layer)
{
    uint *ot = (uint *)layer->ot;
    TextCtx &text = *menu->text;
    WipeBar *bar;
    WipeBar *bar2;
    int level;

    GridPicker_Draw(&g_Arc_CourseCarousel, ot + 2);
    ArcadeMenu_DrawMovie(menu, ot, 0x18, 0xfa, 0x80);
    bar = &D_80050174;
    level = WipeBar_GetLevel(bar);
    if (level > 0) {
        Text_SetFont(&text, &D_80123810);
        text.ot = ot;
        *(uint *)&text.clut = (*(uint *)&text.clut & 0xff9fffff) | 0x200000;
        text.rgb = Rgb_Lerp(D_80052194, D_800521A0, level, 0x80);
        Text_DrawString(&text, (signed char *)D_800F8731, 0x1c, 0xaa, 1);
        Text_DrawString(&text, (signed char *)D_800F8731 + 0x1b, 0x1c, 0xda, 1);
    }
    WipeBar_Draw(bar, ot, 0x18, 0x96);
    WipeBar_Draw(&D_80050190, ot, 0x18, 0xc6);
    CourseInfoPanel_Draw(&g_ArcCourseInfoPanel, ot, &text, 0x82, 0xbe, 0x30);
    bar2 = &D_800501AC;
    level = WipeBar_GetLevel(bar2);
    if (level > 0) {
        Text_SetFont(&text, &D_80123810);
        text.ot = ot;
        *(uint *)&text.clut = (*(uint *)&text.clut & 0xff9fffff) | 0x200000;
        text.rgb = Rgb_Lerp(D_80052194, D_800521A0, level, 0x80);
        Text_DrawString(&text, (signed char *)D_800F8623, 0x2a, 0x186, 1);
    }
    WipeBar_Draw(bar2, ot, 0x26, 0x172);
    CourseRecordPanel_Draw(&g_ArcCourseRecordPanel, ot, &text, 0x50, 0x17c);
    GradientBar_Draw((ArcGradientBar *)D_800F3918, ot + 1, 0x160, 0x174);
    Prim_AddDrawMode(ot + 1, 0x200);
    GradientBar_Draw((ArcGradientBar *)D_800F3938, ot + 4, 0, 0x6a);
    Prim_AddDrawMode(ot + 4, 0x200);
}
}
