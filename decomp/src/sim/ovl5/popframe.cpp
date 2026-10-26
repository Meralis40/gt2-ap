// Reads a CD sector into the stream ring, takes the next queued frame and
// releases it.
#include "common.h"
#include "movie.h"

extern "C" {
// Reads the sector the CD just delivered into the frame being filled.
// Returns 1 once the stream has ended. A sector out of order drops the
// frame until the next frame start.
int MovieStream_ReadSector(MovieStream *s)
{
    MovieSectorHeader hdr;
    unsigned int flags;
    unsigned int sectorIdx;
    unsigned int frameNo;
    unsigned char *dst;
    unsigned char *next;
    short writeIdx;
    short queued;
    short ringSize;
    unsigned int last;
    unsigned int frame;
    unsigned int sector;
    unsigned int count;
    unsigned int ended;

    func_8007CFD4(&hdr, 8);
    {
        unsigned short status = hdr.status;
        unsigned short type = hdr.type;
        if (status != 0x160) {
            return 0;
        }
        if (type != 0x5349) {
            return 0;
        }
    }
    flags = s->flags;
    sectorIdx = s->sectorIdx;
    frameNo = s->frameNo;
    dst = s->dst;
    writeIdx = s->writeIdx;
    queued = s->queued;
    ringSize = s->ringSize;
    next = dst;
    if (flags & 4) {
        return 1;
    }
    last = hdr.lastFrame & 0x3fffffff;
    frame = hdr.frameNo;
    sector = hdr.sector;
    count = hdr.sectorCount;
    if (sector == 0) {
        flags &= ~2;
        if (queued == ringSize) {
            goto drop;
        }
        frameNo = frame;
        dst = s->ring[writeIdx];
        sectorIdx = 0;
        goto accept;
    }
    if (!(flags & 2) && frameNo == frame && sectorIdx == sector) {
        goto accept;
    }
drop:
    flags |= 2;
    if (frame >= last) {
        flags |= 4;
    }
    goto done;
accept:
    sectorIdx++;
    next = dst + 0x7e0;
    if (sector == count - 1) {
        writeIdx++;
        queued++;
        if (writeIdx == ringSize) {
            writeIdx = 0;
        }
        next = s->ring[writeIdx];
        if (frame >= last) {
            flags |= 4;
        }
    }
    func_8007CFD4(dst, 0x1f8);
done:
    s->flags = flags;
    s->sectorIdx = sectorIdx;
    s->frameNo = frameNo;
    s->dst = next;
    s->writeIdx = writeIdx;
    s->queued = queued;
    ended = flags & 4;
    return ended != 0;
}

// Next queued frame, 0 if none. Its slot stays in use until
// MovieStream_ReleaseFrame.
unsigned char *MovieStream_PopFrame(MovieStream *s)
{
    int leave = Sys_CriticalSection(0);
    int idx = s->readIdx;
    int size = s->ringSize;
    unsigned char *frame = 0;

    if (s->queued != 0) {
        frame = s->ring[idx];
        if (++idx == size) {
            idx = 0;
        }
        s->readIdx = idx;
    }
    Sys_CriticalSection(leave);
    return frame;
}

// Frees the slot of the frame taken by MovieStream_PopFrame.
void MovieStream_ReleaseFrame(MovieStream *s)
{
    int leave = Sys_CriticalSection(0);
    int queued = s->queued;

    if (queued != 0) {
        queued--;
    }
    s->queued = queued;
    Sys_CriticalSection(leave);
}
}
