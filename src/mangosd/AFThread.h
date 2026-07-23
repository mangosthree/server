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

#ifndef ANTIFREEZE_THREAD
#define ANTIFREEZE_THREAD

#include "Common.h"
#include "Threading/Threading.h"

/**
 * @brief Watchdog thread that bang-crashes the process if the world loop hangs.
 */
class AntiFreezeThread
{
    public:
        explicit AntiFreezeThread(uint32 delay);
        ~AntiFreezeThread();

        /// Starts the watchdog thread.
        void Activate();

    private:
        /// Runnable body driving the watchdog loop.
        class Body : public MaNGOS::Runnable
        {
            public:
                explicit Body(uint32 delay)
                    : m_loops(0), m_lastchange(0), w_loops(0), w_lastchange(0), m_delayTime(delay)
                {
                }

                void run() override;

            private:
                uint32 m_loops;
                uint32 m_lastchange;
                uint32 w_loops;
                uint32 w_lastchange;
                uint32 m_delayTime;
        };

        uint32          m_delayTime;
        MaNGOS::Thread* m_thread;
};

#endif
