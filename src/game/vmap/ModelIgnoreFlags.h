/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2025 MaNGOS <https://www.getmangos.eu>
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

#ifndef MANGOS_H_MODEL_IGNORE_FLAGS
#define MANGOS_H_MODEL_IGNORE_FLAGS

#include <Platform/Define.h>

namespace VMAP
{
    /// Flag set passed through IsInLineOfSight to selectively skip model
    /// categories during the ray test. The canonical use case is to pass
    /// \c ModelIgnoreFlags::M2 from spell-side LoS so decorative WMO
    /// interior doodads (banners, pews, candle holders) do not block
    /// spells the way structural WMO geometry does. Mirrors TC's
    /// VMAP::ModelIgnoreFlags (Models/ModelIgnoreFlags.h).
    enum class ModelIgnoreFlags : uint32
    {
        Nothing = 0x00, ///< Default — every collision model participates.
        M2      = 0x01  ///< Skip M2 doodads (MOD_M2 entries) during the ray test.
    };

    inline ModelIgnoreFlags operator&(ModelIgnoreFlags left, ModelIgnoreFlags right)
    {
        return ModelIgnoreFlags(uint32(left) & uint32(right));
    }
}

#endif
