/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2021 MaNGOS <https://getmangos.eu>
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
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef TILE_MESSAGE_BLOCK_H
#define TILE_MESSAGE_BLOCK_H

#include "ace/Message_Block.h"
#include "MapBuilder.h"

class TileBuilder
{
    public:
        TileBuilder(MMAP::MapBuilder* builder, int mapID, int tileX, int tileY, dtNavMesh* mesh) :
            m_navMesh(mesh), m_tileY(tileY), m_tileX(tileX), m_mapID(mapID), m_builder(builder) {}
        ~TileBuilder() { delete m_navMesh; }
        void Work() { m_builder->buildTile(m_mapID, m_tileX, m_tileY, m_navMesh); }
    private:
        int m_mapID;
        int m_tileX;
        int m_tileY;
        MMAP::MapBuilder* m_builder;
        dtNavMesh* m_navMesh;
};


class Tile_Message_Block : public ACE_Message_Block
{
    public:
        typedef ACE_Message_Block BASE;

        Tile_Message_Block(TileBuilder* _tileBuilder, size_t size = 0) : BASE(size), m_tileBuilder(_tileBuilder) {}
        ~Tile_Message_Block() { delete m_tileBuilder; }

        TileBuilder* GetTileBuilder() const { return m_tileBuilder; }

    protected:
        Tile_Message_Block& operator=(const Tile_Message_Block&);
        Tile_Message_Block(const Tile_Message_Block&);

        TileBuilder* m_tileBuilder;
};

#endif
