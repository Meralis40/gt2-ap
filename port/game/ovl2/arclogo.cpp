// OVL2 80016cc0-800173d4 (arclogo): Car maker logos.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
extern unsigned int (&g_VolLba);


// Uploads a logo to slot: the archive's image index, or the buffer read from the CD.
void ArcLogo_Upload(ArcLogo *logo, int slot, int index)
{
    int page = logo->vramPage;
    int line = slot << 7;
    unsigned char *tim;
    TimBlock *block;
    RECT rect;
    int x;
    int y;

    if (index < 0) {
        tim = (unsigned char *)logo->timBuf;
        x = page & 0xf;
    } else {
        tim = logo->archive + ((int *)logo->archive)[index + 1];
        x = page & 0xf;
    }
    x <<= 6;
    y = (page & 0x10) << 4;
    block = (TimBlock *)(tim + 8);
    rect.x = x;
    rect.y = y + line;
    rect.w = block->w;
    rect.h = block->h;
    Tim_LoadClut(tim, &rect);
    rect.x = x;
    rect.y = y + (line + 1);
    block = (TimBlock *)((char *)block + ((TimBlock *)(tim + 8))->len);
    rect.w = block->w;
    rect.h = block->h;
    Tim_LoadImage(tim, &rect);
    logo->w[slot] = block->w << 2;
    logo->h[slot] = block->h;
    logo->ready[slot] = 1;
    logo->pending[slot] = 0;
}

// Uploads the pending logos: those of the CD at once, the others with force.
void ArcLogo_UploadPending(ArcLogo *logo, int force)
{
    int i = 0;

    do {
        if (logo->pending[i] != 0) {
            int index = logo->archIdx[i];

            if (index < 0) {
                force = 1;
            }
            if (force != 0) {
                ArcLogo_Upload(logo, i, index);
            }
        }
        i++;
    } while (i < 2);
}

// Sets the page and buffers and resets.
void ArcLogo_Init(ArcLogo *logo, int vramPage, unsigned char *archive, void *timBuf)
{
    logo->vramPage = vramPage;
    logo->archive = archive;
    logo->timBuf = timBuf;
    logo->cdBusy = '\0';
    logo->cancel = '\0';
    ArcLogo_Reset(logo);
}

// Forgets both logos and stops the read.
void ArcLogo_Reset(ArcLogo *logo)
{
    int i;

    for (i = 0; i < 2; i++) {
        logo->carId[i] = 0;
        logo->ready[i] = 0;
        logo->pending[i] = 0;
    }
    logo->cur = 0;
    if (logo->cdBusy != 0) {
        logo->cancel = 1;
    }
    logo->loadState = 0;
}

// Shows carId's logo in the other slot: from the archive, or read from the CD when
// index is negative.
void ArcLogo_Request(ArcLogo *logo, int index, unsigned int carId)
{
    int slot = (logo->cur + 1) & 1;

    logo->cur = slot;
    if ((logo->ready[slot] != 0) & (logo->carId[slot] == carId)) {
        return;
    }
    ArcLogo_CancelRead(logo);
    logo->ready[slot] = 0;
    logo->carId[slot] = carId;
    if (index < 0) {
        logo->loadState = 1;
        logo->archIdx[slot] = -1;
    } else {
        logo->pending[slot] = 1;
        logo->archIdx[slot] = index;
    }
}

// Stops the read.
void ArcLogo_CancelRead(ArcLogo *logo)
{
    if (logo->cdBusy != 0) {
        logo->cancel = 1;
    }
    logo->loadState = 0;
}

// Reads the logo from the CD when the menu's CD read is free.
void ArcLogo_Update(ArcLogo *logo, ArcadeMenu *menu)
{
    AsyncRead *read = &menu->cdRead;
    int status;

    if (logo->cdBusy & logo->cancel) {
        read->busy = 0;
        read->state = 0;
        logo->cancel = 0;
        menu->cdBusy = 0;
        logo->cdBusy = 0;
    }
    switch (logo->loadState) {
    case 1:
        if (logo->cancel == 0 && menu->cdBusy == 0) {
            menu->cdBusy = 1;
            logo->cdBusy = 1;
            logo->loadState++;
        }
        break;
    case 2: {
        int file = CarAsset_Find(logo->carId[logo->cur])->logoFile + 1;
        int sector = g_VolLba + Vol_GetSector(file);
        int size = Vol_GetSize(file);

        if (AsyncRead_Start_OVL2(read, logo->timBuf, sector, size) != 0) {
            logo->loadState++;
        }
        break;
    }
    case 3:
        switch (AsyncRead_Poll_OVL2(read, &status)) {
        case 0:
            logo->loadState = 2;
            break;
        case 2:
            menu->cdBusy = 0;
            logo->cdBusy = 0;
            logo->pending[logo->cur] = 1;
            logo->loadState++;
            break;
        }
        break;
    default:
        logo->loadState = 0;
        break;
    }
}

// carId's logo, when a slot has it: scale is 8.8, 0 or less without scaling; centred
// at x, under y or with bottom above it.
void ArcLogo_Draw(ArcLogo *logo, void *ot, unsigned int carId, int x, int y, int brightness, int bottom, int scale)
{
    int page = logo->vramPage;
    int slot = -1;
    int i;

    for (i = 0; i < 2; i++) {
        if ((logo->carId[i] == carId) & (logo->ready[i] == 1)) {
            slot = i;
            break;
        }
    }
    if (slot >= 0) {
        int line = slot << 7;
        int w = logo->w[slot];
        int h = logo->h[slot];
        int clut = ((page & 0x10) << 10) | (((page & 0xf) << 2) + (slot << 13));

        if (scale <= 0) {
            PrimSprite *sprite = Prim_AddSprite(ot, brightness | (brightness << 8) | (brightness << 16));

            sprite->x = x - (w >> 1);
            sprite->y = y + 2;
            if (bottom != 0) {
                sprite->y = y - (h >> 1);
            }
            sprite->clut = clut;
            sprite->u = 0;
            sprite->v = line + 1;
            sprite->w = w;
            sprite->h = h;
            Prim_AddDrawMode(ot, page);
        } else {
            int sw = w * scale;
            int sh = h * scale;
            PrimFT4 *quad = Prim_AddPolyFT4((uint *)ot, brightness | (brightness << 8) | (brightness << 16));
            int half = sw >> 8;
            short v;

            quad->x2 = quad->x0 = x - half;
            quad->x3 = quad->x1 = x + half;
            if (bottom != 0) {
                quad->y1 = quad->y0 = y - (sh >> 8);
                v = y;
            } else {
                quad->y1 = quad->y0 = y + 2;
                v = y + (sh >> 7) + 2;
            }
            quad->y3 = quad->y2 = v;
            quad->u2 = quad->u0 = 0;
            quad->v1 = quad->v0 = line + 1;
            quad->u3 = quad->u1 = logo->w[slot] - 1;
            quad->clut = clut;
            quad->tpage = page;
            quad->v3 = quad->v2 = logo->h[slot] + line;
        }
    }
}
}
