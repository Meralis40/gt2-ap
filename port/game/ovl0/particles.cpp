// OVL0 8001681c-80016a3c (particles): Particles.
// The functions that don't match yet are the original assembly.
#include "common.h"
#include "gamestate.h"

extern "C" {
extern short g_ParticleFadeTable[16];

// g_RaceSetup, from g_GameState and an offset local (the offset is too big for an addiu).
static inline RaceSetup *RaceSetup_Get()
{
    char *base = (char *)&g_GameState;
    int offset = 0xbf7c;

    return (RaceSetup *)(base + offset);
}

// Clears the 256 particles, chains them into the free list and sets the rates.
void Particles_Init(ParticlePool *pool)
{
    int i;

    memset(pool->particles, 0, sizeof(pool->particles));
    for (i = 0; i < 256; i++) {
        pool->particles[i].next = i + 1;
    }
    pool->particles[255].next = -1;
    pool->activeHead = -1;
    pool->rise = 0x444;
    pool->fadeKind0 = 0x90;
    pool->fadeKind1 = 0x48;
    pool->freeHead = 0;
    pool->growStep = 8;
}

// Takes a free particle (only at 30 Hz, RaceSetup.frameStep > 1) and links it into the
// active list; returns 0 when there is none.
Particle *Particles_Spawn(ParticlePool *pool, int kind)
{
    int index = pool->freeHead;
    int frameStep;
    int oldHead;
    Particle *p;
    int fade;

    if (index < 0 || (frameStep = RaceSetup_Get()->frameStep) < 2) {
        return 0;
    }
    oldHead = pool->activeHead;
    pool->activeHead = index;
    p = &pool->particles[index];
    pool->freeHead = p->next;
    p->next = oldHead;
    p->kind = kind;
    fade = g_ParticleFadeTable[pool->spawnCount];
    if (kind != 0) {
        p->fade = (pool->fadeKind1 * fade) >> 12;
    } else {
        p->fade = (pool->fadeKind0 * fade) >> 12;
    }
    if (++pool->spawnCount == 16) {
        pool->spawnCount = 0;
    }
    return p;
}

// Moves a particle on by one frame; 0 once it has faded out.
static inline int Particle_Age(Particle *p, int rise, int grow)
{
    p->life -= p->fade;
    if (p->life < 0) {
        return 0;
    }
    p->pos[1] += rise;
    p->size += grow;
    return 1;
}

// Ages every active particle (rise, growth) and frees the dead ones.
void Particles_Update(ParticlePool *pool)
{
    int prev = -1;
    int index = pool->activeHead;

    while (index >= 0) {
        // The pool header is as long as a particle: particles[index] is entry[1].
        Particle *entry = (Particle *)pool + index;
        int next = entry[1].next;
        Particle *p = &entry[1];
        int rise = pool->rise;
        int grow = pool->growStep;
        if (Particle_Age(p, rise, grow)) {
            prev = index;
        } else {
            if (prev < 0) {
                pool->activeHead = next;
            } else {
                pool->particles[prev].next = next;
            }
            pool->particles[index].next = pool->freeHead;
            pool->freeHead = index;
        }
        index = next;
    }
}
}
