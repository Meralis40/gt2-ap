// OVL2 800173d4-80017b7c (coursepanel): Slide animation, course info and record panels.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
extern Font D_80123810;
extern unsigned char D_8004F604[];
extern unsigned char D_8004F608[];
extern unsigned char D_8004F60C[];
extern unsigned char D_8004F610[];
extern unsigned char D_8004F614[];
extern char D_800F8775[]; // a number's format
extern char D_800F871B[]; // what's shown without a record


// Hidden.
void SlideAnim_Clear(ArcRecordPanel *p)
{
    p->active = 0;
    p->anim = 0;
}

// Starts the open animation.
void SlideAnim_Open(ArcRecordPanel *p, void *data)
{
    p->active = 1;
    p->anim = 1;
    p->data = data;
}

// Starts the close animation.
void SlideAnim_Close(ArcRecordPanel *p)
{
    p->anim = -1;
}

// Opening: anim up to 12; closing: down to -12.
void CourseInfoPanel_Update(ArcRecordPanel *p)
{
    int anim;
    int above;
    int closing;

    if (p->active == 1) {
        if ((unsigned int)((unsigned short)p->anim - 1) < 11) {
            p->anim++;
        }
        anim = p->anim;
        above = anim >= -11;
        closing = anim < 0;
        if (above & closing) {
            p->anim--;
        }
    }
}

// The animation as 0 to 128; 0 when hidden.
int FUN_OVL2__80017460(ArcRecordPanel *p)
{
    int anim = p->anim;

    if (p->active != 1) {
        return 0;
    }
    return (anim << 7) / 12;
}

// The two values of the course, one below the other.
void CourseInfoPanel_Draw(ArcRecordPanel *p, void *ot, TextCtx *text, int x, int y, int lineHeight)
{
    char buf[64];

    if (p->active == 1) {
        int fade = p->anim;

        if (fade < 0) {
            fade += 12;
        }
        *(unsigned int *)&text->clut = (*(unsigned int *)&text->clut & 0xff9fffff) | 0x200000;
        text->ot = (uint *)ot;
        fade = (fade << 7) / 12;
        Text_SetFont(text, &D_80123810);
        text->rgb = Rgb_Lerp(D_8004F604, D_8004F614, fade, 0x80);
        sprintf(buf, D_800F8775, ((short *)p->data)[0]);
        Text_DrawStringMonoDigitsRight(text, (signed char *)buf, x, y, 1, -2, 0);
        sprintf(buf, D_800F8775, ((short *)p->data)[2]);
        Text_DrawStringMonoDigitsRight(text, (signed char *)buf, x, y + lineHeight, 1, -2, 0);
    }
}

// Hides the panel (active = 0, anim = 0).
void CourseRecordPanel_Init(ArcRecordPanel *p)
{
    p->active = '\0';
    p->anim = 0;
}

// Shows the best-time record of a course (0x24-byte entry of the course record table
// at 0x801c9af8) and starts the open animation.
void CourseRecordPanel_Open(ArcRecordPanel *p, void *record)
{
    p->anim = 1;
    p->active = '\x01';
    p->data = record;
}

// Starts the close animation (anim -1, then Update counts down to -12).
void CourseRecordPanel_Close(ArcRecordPanel *p)
{
    p->anim = -1;
}

// Same as CourseInfoPanel_Update.
void CourseRecordPanel_Update(ArcRecordPanel *p)
{
    int anim;
    int above;
    int closing;

    if (p->active == 1) {
        if ((unsigned int)((unsigned short)p->anim - 1) < 11) {
            p->anim++;
        }
        anim = p->anim;
        above = anim >= -11;
        closing = anim < 0;
        if (above & closing) {
            p->anim--;
        }
    }
}

// Same as FUN_OVL2__80017460.
int FUN_OVL2__80017690(ArcRecordPanel *p)
{
    int anim = p->anim;

    if (p->active != 1) {
        return 0;
    }
    return (anim << 7) / 12;
}

// The sector times and the total of the record, then its car and driver.
void CourseRecordPanel_Draw(ArcRecordPanel *p, void *ot, TextCtx *text, int x, int y)
{
    char buf[256];
    int line1 = y + 0x18;
    int line2 = y + 0x30;

    if (p->active == 1) {
        int fade = p->anim;
        CourseRecord *record;
        unsigned int time;
        unsigned int rgb;
        int width;
        RECT *tile;
        int i;

        if (fade < 0) {
            fade += 12;
        }
        *(unsigned int *)&text->clut = (*(unsigned int *)&text->clut & 0xff9fffff) | 0x200000;
        text->ot = (uint *)ot;
        fade = (fade << 7) / 12;
        Text_SetFont(text, &D_80123810);
        text->rgb = Rgb_Lerp(D_8004F604, D_8004F608, fade, 0x80);
        for (i = 0; i < 5; i++) {
            record = (CourseRecord *)p->data;
            time = record->best.time;
            if (i < 4) {
                time = Records_GetSectorTime(&record->best, i);
            }
            if (i == 4) {
                text->rgb = Rgb_Lerp(D_8004F604, D_8004F60C, fade, 0x80);
            }
            Time_FormatMs(buf, time);
            Text_DrawDigitsRight(text, (signed char *)buf, x, line2, 6, 5, 0, 0);
            x += 0x3c;
        }
        record = (CourseRecord *)p->data;
        x -= 0x39;
        if (~record->best.time != 0) {
            char *name = CarInfoA_GetName(*(unsigned int *)((char *)record + 0x14));

            rgb = Rgb_Lerp(D_8004F604, D_8004F610, fade, 0x80);
            text->rgb = rgb;
            width = Text_DrawStringRight(text, (signed char *)name, x, line1, 1);
            tile = Prim_AddTile(ot, rgb);
            tile->x = x - width - 8;
            tile->y = line1 - 0xc;
            tile->w = 4;
            tile->h = 6;
            Prim_AddDrawMode(ot, 0x220);
            name = (char *)p->data + 0x18;
            rgb = Rgb_Lerp(D_8004F604, D_8004F610, fade, 0x80);
            text->rgb = rgb;
            width = Text_DrawStringRight(text, (signed char *)name, x, y, 1);
            tile = Prim_AddTile(ot, rgb);
            tile->x = x - width - 8;
            tile->y = y - 0xc;
            tile->w = 4;
            tile->h = 6;
            Prim_AddDrawMode(ot, 0x220);
        } else {
            text->rgb = Rgb_Lerp(D_8004F604, D_8004F610, fade, 0x80);
            Text_DrawStringRight(text, (signed char *)D_800F871B, x, line1, 1);
        }
    }
}

// The course map's menu hook: decompresses the map and uploads it. 1 while it runs.
unsigned char FUN_OVL2__80017a14(void *arg, int active, int step)
{
    ArcCourseMap *cm = (ArcCourseMap *)arg;
    int page = (signed char)cm->tpage;
    unsigned char *tim = (unsigned char *)cm->buf2;
    RECT rect;
    TimBlock *block;
    int x;
    int y;

    if (active == 0) {
        cm->hookActive = 0;
        return 0;
    }
    switch (cm->unk1d) {
    case 0:
        if (step != 0 && cm->loaded != 0) {
            cm->unk1d = 1;
        }
        return 1;
    case 1:
        if (step != 0) {
            Lz_Decompress(tim, (unsigned char *)cm->buf + 0x10, 0x49b0);
            cm->unk1d = 2;
        }
        return 1;
    case 2:
        if (step == 1) {
            return 1;
        }
        x = (page & 0xf) << 6;
        y = (page << 4) & 0x100;
        block = (TimBlock *)(tim + 8);
        rect.x = x;
        rect.y = y;
        rect.w = block->w;
        rect.h = block->h;
        Tim_LoadClut(tim, &rect);
        y++;
        rect.x = x;
        rect.y = y;
        block = (TimBlock *)((char *)block + ((TimBlock *)(tim + 8))->len);
        rect.w = block->w;
        rect.h = block->h;
        Tim_LoadImage(tim, &rect);
        cm->decoded = 1;
        cm->hookActive = 0;
        return 0;
    }
    return 1;
}
}
