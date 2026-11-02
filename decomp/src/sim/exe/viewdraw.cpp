// Resident code 80080088-800804f8 (viewdraw): a view's ordering table, split into
// layers sorted by priority, each starting with a packet that sets its drawing area,
// and the double-buffered pool the primitives are allocated from.
#define INCLUDE_ASM_USE_MACRO_INC 1
#include "include_asm.h"
#include "resident.h"

extern "C" {
extern uint *g_PacketPtr;
extern uint *g_PacketEnd;
extern RECT g_DrawArea;

void Gpu_DrawOTag(void *list);
void Dma_ClearOTagR(void *ot, int n);
void ViewDrawContext_Reset(ViewDrawContext *ctx);
void DrawLayer_SetArea(DrawLayer *layer, RECT *area);

void ViewDrawContext_Init(ViewDrawContext *ctx, void *ot, unsigned int bytes)
{
    ctx->otBase = ot;
    ctx->otSize = (unsigned short)(bytes >> 2);
    ctx->drawMode = 0;
    ViewDrawContext_Reset(ctx);
}

// Inserts layer after the last layer of the same or a lower priority. The layers after
// it are dropped from the list.
void ViewDrawContext_AddLayer(ViewDrawContext *ctx, DrawLayer *layer)
{
    DrawLayer *prev = 0;
    DrawLayer *l;
    DrawLayer *next = ctx->layers;
    int priority = layer->priority;

    while ((l = next) != 0) {
        next = l->next;
        if (priority < l->priority) {
            break;
        }
        prev = l;
    }
    layer->next = 0;
    if (prev == 0) {
        ctx->layers = layer;
    } else {
        prev->next = layer;
    }
}

void ViewDrawContext_Reset(ViewDrawContext *ctx)
{
    ctx->layers = 0;
    ctx->otHead = 0;
    ctx->otHalf = 0;
}

// Gives each layer its slice of the current half of the ordering table.
void ViewDrawContext_AssignOT(ViewDrawContext *ctx)
{
    DrawLayer *l;
    DrawLayer *next = ctx->layers;
    uint *ot = (uint *)ctx->otStart;

    while ((l = next) != 0) {
        next = l->next;
        l->ot = ot;
        ot += l->otSize;
    }
}

// Starts a frame: reserves the packets ViewDrawContext_End fills in, links an empty
// packet at the start of the table, and gives each layer its slice and a header that
// sets its drawing area to area, relative to the draw buffer.
// The 24-bit links are copied with lwl and swl, one asm statement each.
void ViewDrawContext_Begin(ViewDrawContext *ctx, RECT *area)
{
    uint *p;
    OtTag *ot;
    uint link;
    LayerHeader *header;
    uint *slice;
    DrawLayer *l;
    DrawLayer *next;
    uint tag;

    p = g_PacketPtr;
    ctx->tailPackets = p;
    p += 4;
    tag = (uint)p << 8;
    ((uchar *)p)[3] = 1;
    p[1] = 0;
    ot = (OtTag *)ctx->otBase;
    __asm__ volatile("lwl\t%0,2(%1)" : "=r"(link) : "r"(ot));
    __asm__ volatile("swl\t%0,2(%1)" : : "r"(tag), "r"(ot) : "memory");
    __asm__ volatile("swl\t%0,2(%1)" : : "r"(link), "r"(p) : "memory");
    header = (LayerHeader *)(p + 2);
    next = ctx->layers;
    slice = (uint *)ctx->otStart;
    while ((l = next) != 0) {
        l->header = header;
        header++;
        l->ot = slice;
        DrawLayer_SetArea(l, area);
        slice += l->otSize;
        next = l->next;
    }
    g_PacketPtr = (uint *)header;
}

// Ends a frame: links each layer's header after the last entry of its slice, so the
// GPU sets the layer's area before its primitives, then a cache flush and a draw-mode
// packet at the end of the table.
#ifdef NON_MATCHING
// The game has the last swl of each link in the delay slot of the loop's j and of the
// jr, where GCC never puts an asm statement, and it loads the header before computing
// the slice's last entry. With "m" operands the asm statements get the game's offsets
// (-2 from the end of the slice, 10 from the packets for the second one).
void ViewDrawContext_End(ViewDrawContext *ctx)
{
    DrawLayer *l;
    DrawLayer *next = ctx->layers;
    uint *p;
    OtTag *ot;
    uint mode;
    uint link;

    while ((l = next) != 0) {
        OtTag *last;
        LayerHeader *header;

        next = l->next;
        last = (OtTag *)l->ot + l->otSize - 1;
        header = l->header;
        __asm__ volatile("lwl\t%0,%1" : "=r"(link) : "m"(((char *)last)[2]));
        ((uchar *)header)[3] = 3;
        __asm__ volatile("swl\t%1,%0" : "=m"(((char *)header)[2]) : "r"(link));
        __asm__ volatile("swl\t%1,%0" : "=m"(((char *)last)[2]) : "r"((uint)header << 8));
    }
    ot = (OtTag *)ctx->otEnd;
    mode = ctx->drawMode;
    p = (uint *)ctx->tailPackets;
    p[1] = 0x1000000;
    __asm__ volatile("lwl\t%0,%1" : "=r"(link) : "m"(((char *)ot)[2]));
    ((uchar *)p)[3] = 1;
    __asm__ volatile("swl\t%1,%0" : "=m"(((char *)p)[2]) : "r"(link));
    __asm__ volatile("swl\t%1,%0" : "=m"(((char *)ot)[2]) : "r"((uint)p << 8));
    p[3] = mode ^ 0xe1000200;
    __asm__ volatile("lwl\t%0,%1" : "=r"(link) : "m"(((char *)ot)[2]));
    ((uchar *)p)[11] = 1;
    __asm__ volatile("swl\t%1,%0" : "=m"(((char *)(p + 2))[2]) : "r"(link));
    __asm__ volatile("swl\t%1,%0" : "=m"(((char *)ot)[2]) : "r"((uint)(p + 2) << 8));
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", ViewDrawContext_End);
#endif

void ViewDrawContext_Submit(ViewDrawContext *ctx)
{
    if (ctx->otHead != 0) {
        Gpu_DrawOTag(ctx->otHead);
    }
}

// Switches to the other half of the ordering table; otHead is the last entry of the
// half just built.
#ifdef NON_MATCHING
// Only registers differ: the game keeps the size in $v0 and otHalf in $v1, then reuses
// $v1 for the index, where GCC swaps the first two. The order of the declarations, int
// or ushort locals, testing ctx->otHalf directly and other forms of the index didn't
// change that.
void ViewDrawContext_SwapOT(ViewDrawContext *ctx)
{
    uint half = 0;
    uint n = ctx->otSize;
    uint cur = ctx->otHalf;
    uint *base = (uint *)ctx->otBase;

    n >>= 1;
    if (cur == 0) {
        half = n;
    }
    n = half + n - 1;
    ctx->otHalf = half;
    ctx->otHead = base + n;
}
#else
INCLUDE_ASM("asm/sim/exe/nonmatchings/viewdraw", ViewDrawContext_SwapOT);
#endif

void ViewDrawContext_ClearOT(ViewDrawContext *ctx)
{
    uint *start = (uint *)ctx->otBase + ctx->otHalf;
    uint n = ctx->otSize >> 1;

    ctx->otStart = start;
    ctx->otEnd = start + n - 1;
    Dma_ClearOTagR(start, n);
}

// Fills the layer's header: area, relative to the draw buffer and clipped to it, as
// its drawing area, and its top left corner as the drawing offset.
void DrawLayer_SetArea(DrawLayer *layer, RECT *area)
{
    LayerHeader *header = layer->header;
    int left = g_DrawArea.x;
    int top = g_DrawArea.y;
    int x0 = left + area->x;
    int y0 = top + area->y;
    int x1 = x0 + area->w;
    int y1 = y0 + area->h;
    int right = left + g_DrawArea.w;
    int bottom = top + g_DrawArea.h;
    uint offset = ((y0 & 0x7ff) << 11 | (x0 & 0x7ff)) | 0xe5000000;

    if (x0 < left) {
        x0 = left;
    }
    if (y0 < top) {
        y0 = top;
    }
    if (x1 < left) {
        x1 = left;
    }
    if (y1 < top) {
        y1 = top;
    }
    if (x0 >= right) {
        x0 = right;
    }
    if (y0 >= bottom) {
        y0 = bottom;
    }
    if (x1 >= right) {
        x1 = right;
    }
    x1--;
    if (y1 >= bottom) {
        y1 = bottom;
    }
    y1--;
    header->drawAreaTopLeft = ((y0 & 0x3ff) << 10 | (x0 & 0x3ff)) | 0xe3000000;
    header->drawAreaBottomRight = ((y1 & 0x3ff) << 10 | (x1 & 0x3ff)) | 0xe4000000;
    header->drawOffset = offset;
}

// Links a draw-mode packet followed by a sprite of colour rgb in after ot, and returns
// the draw-mode word for the caller to fill in with the rest.
void *Prim_AddTexQuad(OtTag *ot, uint rgb)
{
    uint *p;
    uint link;

    __asm__ volatile("lwl\t%0,2(%1)" : "=r"(link) : "r"(ot));
    p = g_PacketPtr;
    ((uchar *)p)[3] = 5;
    __asm__ volatile("swl\t%0,2(%1)" : : "r"(link), "r"(p) : "memory");
    __asm__ volatile("swl\t%0,2(%1)" : : "r"((uint)p << 8), "r"(ot) : "memory");
    p[1] = 0xe1000000;
    p[2] = rgb ^ 0x64000000;
    g_PacketPtr = p + 6;
    return p + 1;
}

void PacketBuffer_Init(PacketBuffer *buf, void *base, uint bytes)
{
    buf->base = base;
    buf->size = bytes >> 2;
    buf->half = 0;
    g_PacketPtr = (uint *)buf->base;
}

// Switches to the other half of the pool.
void PacketBuffer_Swap(PacketBuffer *buf)
{
    uint *base = (uint *)buf->base;
    uint size = buf->size;
    uint half = 0;

    if (buf->half == 0) {
        half = size >> 1;
    }
    buf->half = half;
    g_PacketPtr = base + half;
    g_PacketEnd = base + half + (size >> 1);
}
}
