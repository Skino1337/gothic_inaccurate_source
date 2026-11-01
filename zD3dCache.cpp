#include "include/zRndD3d.h"

zCDXTCCache_D3D::zCDXTCCache_D3D()
    : xdxtc_cacheentryCounter(0)
{
    for (int pool = 0; pool < 2; ++pool) {
        for (int x = 0; x < 12; ++x) {
            for (int y = 0; y < 12; ++y) {
                xdxtc_slot[pool][x][y].firstEntry = 0;
                xdxtc_slot[pool][x][y].lastEntry = 0;
            }
        }
    }
}

zCDXTCCache_D3D::~zCDXTCCache_D3D()
{
    ClearCache();
}

IDirectDrawSurface7* zCDXTCCache_D3D::Lock(int xdim, int ydim, int format)
{
    zD3D_SlotHeader& slot = xdxtc_slot[format != 10][xdim][ydim];
    zD3D_Entry* entry = slot.firstEntry;
    zD3D_Entry* last = 0;
    while (entry) {
        if (!entry->locked) {
            entry->locked = 1;
            return entry->surface;
        }
        last = entry;
        entry = entry->nextEntry;
    }
    entry = new zD3D_Entry;
    entry->nextEntry = 0;
    entry->locked = 1;
    if (last)
        last->nextEntry = entry;
    else
        slot.firstEntry = entry;
    slot.lastEntry = entry;
    ++xdxtc_cacheentryCounter;
    return XDXTC_CreateSurface(xdim, ydim, format, entry);
}

int zCDXTCCache_D3D::Unlock(int xdim, int ydim, int format, IDirectDrawSurface7* surface)
{
    zD3D_Entry* entry = xdxtc_slot[format != 10][xdim][ydim].firstEntry;
    while (entry) {
        if (entry->surface == surface) {
            entry->locked = 0;
            return 1;
        }
        entry = entry->nextEntry;
    }
    return 0;
}

void zCDXTCCache_D3D::ClearCache()
{
    for (int pool = 0; pool < 2; ++pool) {
        for (int x = 0; x < 12; ++x) {
            for (int y = 0; y < 12; ++y) {
                zD3D_Entry* entry = xdxtc_slot[pool][x][y].firstEntry;
                while (entry) {
                    zD3D_Entry* next = entry->nextEntry;
                    delete[] reinterpret_cast<unsigned char*>(entry->surface);
                    delete entry;
                    entry = next;
                    --xdxtc_cacheentryCounter;
                }
                xdxtc_slot[pool][x][y].firstEntry = 0;
                xdxtc_slot[pool][x][y].lastEntry = 0;
            }
        }
    }
}

IDirectDrawSurface7* zCDXTCCache_D3D::XDXTC_CreateSurface(int xdim, int ydim, int,
    zD3D_Entry* entry)
{
    const unsigned long size = (1UL << xdim) * (1UL << ydim) * 4UL;
    entry->surface = reinterpret_cast<IDirectDrawSurface7*>(new unsigned char[size]);
    return entry->surface;
}
