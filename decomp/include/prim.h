// GPU primitives as the game builds them. The PolyXX structs are whole packets
// (PsyQ's POLY_XX: the OT tag, then the command word and the vertices, as the
// hand-written drawing code fills them); the PrimXX structs are what the
// Prim_AddXX functions return, the packet after its tag.
#ifndef PRIM_H
#define PRIM_H

// An OT entry or packet tag: the next packet's address and this packet's length
// in words.
struct OtTag {
    unsigned addr : 24;
    unsigned len : 8;
};

// A rectangle filled with a gradient (Menu_AddGouraudQuad, Menu_DrawCursorBox),
// or a colour swatch (Menu_DrawColorSwatch, top colour only).
struct GouraudQuad {
    RECT rect;
    unsigned int top;
    unsigned int bottom;
};

// Packets of the hand-written model and clipping code, xy words as the GTE
// stores them.
struct PolyF3 {
    unsigned int tag;
    unsigned int rgbc;
    int xy0;
    int xy1;
    int xy2;
};

struct PolyF4 {
    unsigned int tag;
    unsigned int rgbc;
    int xy0;
    int xy1;
    int xy2;
    int xy3;
};

struct PolyFT3 {
    unsigned int tag;
    unsigned int rgbc;
    int xy0;
    unsigned short uv0;
    unsigned short clut;
    int xy1;
    unsigned short uv1;
    unsigned short tpage;
    int xy2;
    unsigned short uv2;
    unsigned short pad1e;
};

struct PolyFT4 {
    unsigned int tag;
    unsigned int rgbc;
    int xy0;
    unsigned short uv0;
    unsigned short clut;
    int xy1;
    unsigned short uv1;
    unsigned short tpage;
    int xy2;
    unsigned short uv2;
    unsigned short pad1e;
    int xy3;
    unsigned short uv3;
    unsigned short pad26;
};

struct PolyG3 {
    unsigned int tag;
    unsigned int rgbc0;
    int xy0;
    unsigned int rgb1;
    int xy1;
    unsigned int rgb2;
    int xy2;
};

struct PolyG4 {
    unsigned int tag;
    unsigned int rgbc0;
    int xy0;
    unsigned int rgb1;
    int xy1;
    unsigned int rgb2;
    int xy2;
    unsigned int rgb3;
    int xy3;
};

struct PolyGT3 {
    unsigned int tag;
    unsigned int rgbc0;
    int xy0;
    unsigned short uv0;
    unsigned short clut;
    unsigned int rgb1;
    int xy1;
    unsigned short uv1;
    unsigned short tpage;
    unsigned int rgb2;
    int xy2;
    unsigned short uv2;
    unsigned short pad26;
};

struct PolyGT4 {
    unsigned int tag;
    unsigned int rgbc0;
    int xy0;
    unsigned short uv0;
    unsigned short clut;
    unsigned int rgb1;
    int xy1;
    unsigned short uv1;
    unsigned short tpage;
    unsigned int rgb2;
    int xy2;
    unsigned short uv2;
    unsigned short pad26;
    unsigned int rgb3;
    int xy3;
    unsigned short uv3;
    unsigned short pad32;
};

// A draw mode packet (E1 command).
struct DrawMode {
    unsigned int tag;
    unsigned int code;
};

// A TILE and an SPRT, tag included (the menu background's display list).
struct BgTile {
    unsigned int tag;
    unsigned int rgbc;
    short x;
    short y;
    short w;
    short h;
};

struct BgSprite {
    unsigned int tag;
    unsigned int rgbc;
    short x;
    short y;
    unsigned char u;
    unsigned char v;
    unsigned short clut;
    short w;
    short h;
};

// The flat triangle Prim_AddPolyF3 returns.
struct PrimF3 {
    unsigned int rgb;
    short x0, y0;
    short x1, y1;
    short x2, y2;
};

// The Gouraud triangle Prim_AddPolyG3 returns.
struct PrimG3 {
    unsigned int c0;
    short x0, y0;
    unsigned int c1;
    short x1, y1;
    unsigned int c2;
    short x2, y2;
};

// The Gouraud quad Prim_AddPolyG4 returns.
struct PrimG4 {
    unsigned int c0;
    short x0, y0;
    unsigned int c1;
    short x1, y1;
    unsigned int c2;
    short x2, y2;
    unsigned int c3;
    short x3, y3;
};

// The textured quad Prim_AddPolyFT4 returns.
struct PrimFT4 {
    unsigned int rgb;
    short x0, y0;
    unsigned char u0, v0;
    unsigned short clut;
    short x1, y1;
    unsigned char u1, v1;
    unsigned short tpage;
    short x2, y2;
    unsigned char u2, v2;
    unsigned short pad1;
    short x3, y3;
    unsigned char u3, v3;
    unsigned short pad2;
};

// The same quad with its texture coordinates as words (the kanji font's glyphs),
// and with its tag.
struct KanjiQuad {
    unsigned int rgb;
    short x0, y0;
    unsigned short uv0, clut;
    short x1, y1;
    unsigned short uv1, tpage;
    short x2, y2;
    unsigned short uv2, pad1;
    short x3, y3;
    unsigned short uv3, pad2;
};

struct KanjiPrim {
    unsigned char addr[3]; // the OT link
    unsigned char len;
    KanjiQuad quad;
};

// The part of the textured quad Prim_AddTexQuad returns that the boot logo sets.
struct TexQuad {
    short x;
    short unk02[3];
    short texX;
    short y;
    unsigned char u;
    unsigned char v;
    unsigned short clut;
    short w;
    short h;
};

// The SPRT that Prim_AddSprite returns (from its xy).
struct PrimSprite {
    short x;
    short y;
    unsigned char u;
    unsigned char v;
    unsigned short clut;
    short w;
    short h;
};

// A flat line's vertices (Prim_AddLineF2).
struct LineF2 {
    short x0, y0;
    short x1, y1;
};

// The LINE_F5 that Prim_AddLineF4 returns: a closed outline of 4 points.
struct PrimLineF4 {
    unsigned int rgbc;
    short x0;
    short y0;
    short x1;
    short y1;
    short x2;
    short y2;
    short x3;
    short y3;
    short x4;
    short y4;
};

#endif
