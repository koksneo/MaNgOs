/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "GridMap.h"
#include "Log.h"
#include "World.h"

#include "MoveMap.h"
#include "MoveMapSharedDefines.h"

uint32 packTileID(int x, int y) { return uint32(x << 16 | y); }

void TerrainInfo::LoadNavMesh(int gx, int gy)
{
    // load and allocate map's mesh
    if (!m_navMesh)
    {
        uint32 pathLen = sWorld.GetDataPath().length() + strlen("mmaps/%03i.mmap")+1;
        char *fileName = new char[pathLen];
        snprintf(fileName, pathLen, (sWorld.GetDataPath()+"mmaps/%03i.mmap").c_str(), m_mapId);

        FILE* file = fopen(fileName, "rb");
        if (!file)
        {
            sLog.outDebug("MMAP: Error: Could not open mmap file '%s'", fileName);
            delete [] fileName;
            return;
        }

        dtNavMeshParams params;
        fread(&params, sizeof(dtNavMeshParams), 1, file);
        fclose(file);

        m_navMesh = dtAllocNavMesh();
        if (!m_navMesh->init(&params))
        {
            dtFreeNavMesh(m_navMesh);
            m_navMesh = NULL;
            sLog.outError("MMAP: Failed to initialize mmap %03u from file %s", m_mapId, fileName);
            delete [] fileName;
            return;
        }

        delete [] fileName;
    }

    // allocate mesh query
    if(!m_navMeshQuery)
    {
        m_navMeshQuery = dtAllocNavMeshQuery();
        MANGOS_ASSERT(m_navMeshQuery);
        if(DT_SUCCESS != m_navMeshQuery->init(m_navMesh, 2048))
        {
            sLog.outError("MMAP: Failed to allocate dtNavMeshQuery for %03u%02i%02i.mmtile", m_mapId, gx, gy);
            return;
        }
    }

    // check if we already have this tile loaded
    uint32 packedGridPos = packTileID(gx, gy);
    if (m_mmapLoadedTiles.find(packedGridPos) != m_mmapLoadedTiles.end())
    {
        sLog.outError("MMAP: Asked to load already loaded navmesh tile. %03u%02i%02i.mmtile", m_mapId, gx, gy);
        return;
    }

    // mmaps/0000000.mmtile
    uint32 pathLen = sWorld.GetDataPath().length() + strlen("mmaps/%03i%02i%02i.mmtile")+1;
    char *fileName = new char[pathLen];
    snprintf(fileName, pathLen, (sWorld.GetDataPath()+"mmaps/%03i%02i%02i.mmtile").c_str(), m_mapId, gx, gy);

    FILE *file = fopen(fileName, "rb");
    if (!file)
    {
        sLog.outDebug("MMAP: Could not open mmtile file '%s'", fileName);
        delete [] fileName;
        return;
    }
    delete [] fileName;

    // read header
    MmapTileHeader fileHeader;
    fread(&fileHeader, sizeof(MmapTileHeader), 1, file);

    if (fileHeader.mmapMagic != MMAP_MAGIC)
    {
        sLog.outError("MMAP: Bad header in mmap %03u%02i%02i.mmtile", m_mapId, gx, gy);
        return;
    }

    if (fileHeader.mmapVersion != MMAP_VERSION)
    {
        sLog.outError("MMAP: %03u%02i%02i.mmtile was built with generator v%i, expected v%i",
                                            m_mapId, gx, gy, fileHeader.mmapVersion, MMAP_VERSION);
        return;
    }

    unsigned char* data = (unsigned char*)dtAlloc(fileHeader.size, DT_ALLOC_PERM);
    MANGOS_ASSERT(data);

    size_t result = fread(data, fileHeader.size, 1, file);
    if(!result)
    {
        sLog.outError("MMAP: Bad header or data in mmap %03u%02i%02i.mmtile", m_mapId, gx, gy);
        fclose(file);
        return;
    }

    fclose(file);

    dtMeshHeader* header = (dtMeshHeader*)data;
    dtTileRef tileRef = 0;

    // memory allocated for data is now managed by detour, and will be deallocated when the tile is removed
    dtStatus dtResult = m_navMesh->addTile(data, fileHeader.size, DT_TILE_FREE_DATA, 0, &tileRef);
    switch(dtResult)
    {
        case DT_SUCCESS:
        {
            m_mmapLoadedTiles.insert(std::pair<uint32, dtTileRef>(packedGridPos, tileRef));
            sLog.outDetail("MMAP: Loaded mmtile %03i[%02i,%02i] into %03i[%02i,%02i]", m_mapId, gx, gy, m_mapId, header->x, header->y);
        }
        break;
        case DT_FAILURE_DATA_MAGIC:     // those are kept and checked in our mmtile file headers
        case DT_FAILURE_DATA_VERSION:
        case DT_FAILURE_OUT_OF_MEMORY:
        case DT_FAILURE:
        default:
        {
            sLog.outError("MMAP: Could not load %03u%02i%02i.mmtile into navmesh", m_mapId, gx, gy);
            dtFree(data);
        }
        break;
    }
}

void TerrainInfo::UnloadNavMesh(int gx, int gy)
{
    // navMesh was not loaded for this map
    if (!m_navMesh)
        return;

    uint32 packedGridPos = packTileID(gx, gy);
    if (m_mmapLoadedTiles.find(packedGridPos) == m_mmapLoadedTiles.end())
    {
        // file may not exist, therefore not loaded
        sLog.outDebug("MMAP: Asked to unload not loaded navmesh tile. %03u%02i%02i.mmtile", m_mapId, gx, gy);
        return;
    }

    dtTileRef tileRef = m_mmapLoadedTiles[packedGridPos];

    // unload, and mark as non loaded
    if(DT_SUCCESS != m_navMesh->removeTile(tileRef, NULL, NULL))
    {
        // because the Terrain unloads the grid, this is technically a memory leak
        // if the grid is later reloaded, dtNavMesh::addTile will return error but no extra memory is used
        // we cannot recover from this error - assert out
        sLog.outError("MMAP: Could not unload %03u%02i%02i.mmtile from navmesh", m_mapId, gx, gy);
        MANGOS_ASSERT(false);
    }
    else
    {
        m_mmapLoadedTiles.erase(packedGridPos);
        sLog.outDetail("MMAP: Unloaded mmtile %03i[%02i,%02i] from %03i", m_mapId, gx, gy, m_mapId);
    }
}

dtNavMesh const* TerrainInfo::GetNavMesh() const
{
    return m_navMesh;
}

dtNavMeshQuery const* TerrainInfo::GetNavMeshQuery() const
{
    return m_navMeshQuery;
}

std::set<uint32> TerrainInfo::s_mmapDisabledIds = std::set<uint32>();

void TerrainInfo::preventPathfindingOnMaps(std::string ignoreMapIds)
{
    s_mmapDisabledIds.clear();

    uint32 strLenght = ignoreMapIds.length()+1;
    char* mapList = new char[strLenght];
    memcpy(mapList, ignoreMapIds.c_str(), sizeof(char)*strLenght);

    char* idstr = strtok(mapList, ",");
    while (idstr)
    {
        s_mmapDisabledIds.insert(uint32(atoi(idstr)));
        idstr = strtok(NULL, ",");
    }

    delete[] mapList;
}

bool TerrainInfo::IsPathfindingEnabled() const
{
    return sWorld.getConfig(CONFIG_BOOL_MMAP_ENABLED) && s_mmapDisabledIds.find(m_mapId) == s_mmapDisabledIds.end();
}
