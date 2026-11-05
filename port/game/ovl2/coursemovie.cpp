// OVL2 8001802c-8001824c (coursemovie): Course preview movie.
// Ghidra's decompilation; the functions that don't match yet are the original
// assembly.
#include "common.h"

extern "C" {
extern ArcStrPlayer g_Arc_PreviewPlayer;


// Course preview movie (g_ArcCoursePreviewMovie at 0x80129850): sets the decode
// buffer, idle, no movie.
void CoursePreviewMovie_Init(ArcCoursePreviewMovie *m, void *buffer)
{
    m->buffer = buffer;
    m->cdClaimed = '\0';
    m->stopRequested = '\0';
    m->started = '\0';
    m->movie = -1;
    m->state = '\0';
}

// Requests movie index movie (state 1).
void CoursePreviewMovie_Play(ArcCoursePreviewMovie *m, void *menu, int movie)
{
    m->movie = movie;
    m->state = '\x01';
}

// Stops the movie; the player closes on the next update when it holds the CD.
void CoursePreviewMovie_Stop(ArcCoursePreviewMovie *m, void *menu)
{
    if (m->cdClaimed != 0) {
        m->stopRequested = 1;
    }
    m->state = 0;
}

// Waits for the menu's CD read to be free, starts the player, or closes it.
void CoursePreviewMovie_Update(ArcCoursePreviewMovie *m, ArcadeMenu *menu)
{
    if (m->cdClaimed & m->stopRequested) {
        if (m->started != 0) {
            StrPlayer_Close(&g_Arc_PreviewPlayer);
            m->started = 0;
        }
        menu->cdBusy = 0;
        m->cdClaimed = 0;
        m->stopRequested = 0;
        return;
    }
    switch (m->state) {
    case 1:
        if (m->stopRequested == 0 && menu->cdBusy == 0) {
            menu->cdBusy = 1;
            m->cdClaimed = 1;
            m->state++;
        }
        break;
    case 2:
        Movie_InitRing_OVL2(&g_Arc_PreviewPlayer, m->movie, m->buffer);
        m->started = 1;
        m->state++;
        break;
    case 3:
        break;
    default:
        m->state = 0;
        break;
    }
}

static inline int IsStarted(ArcCoursePreviewMovie *m)
{
    return m->started;
}

// One frame of the player while the movie plays.
void CoursePreviewMovie_Decode(ArcCoursePreviewMovie *m, ArcadeMenu *menu)
{
    if (m->state == 3 && (IsStarted(m) & 1)) {
        StrPlayer_Poll(&g_Arc_PreviewPlayer);
    }
}

// The movie's frame, 0x70 by 0x60, when one is decoded.
void CoursePreviewMovie_Draw(ArcCoursePreviewMovie *m, void *ot, int x, int y, int unused)
{
    TexQuad *q;
    int show = 0;
    int done = g_Arc_PreviewPlayer.frameDone != 0;

    if (m->state == 3) {
        show = done & 1;
    }
    if (show) {
        q = (TexQuad *)Prim_AddTexQuad((OtTag *)ot, 0x1808080);
        q->x = 0x11a;
        q->texX = x;
        q->y = y + 4;
        q->w = 0x70;
        q->u = 0;
        q->v = 0;
        q->clut = 0;
        q->h = 0x60;
    }
}
}
