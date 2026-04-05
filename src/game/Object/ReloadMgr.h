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

#ifndef __RELOADMGR_H
#define __RELOADMGR_H

#include "Common.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

class ReloadMgr
{
    public:
        static ReloadMgr& instance();

        ReloadMgr();
        ~ReloadMgr();

        void StartFullReload(void (*printFn)(void*, char const*), void* printArg, bool forceRespawn);

        bool IsReloading() const { return m_running.load(); }

    private:
        struct ReloadTask
        {
            void (*printFn)(void*, char const*);
            void* printArg;
            bool  forceRespawn;
        };

        void WorkerThread();

        std::thread                m_worker;
        std::atomic<bool>          m_running = {false};
        std::mutex                 m_queueLock;
        std::condition_variable    m_cv;
        std::queue<ReloadTask>     m_queue;
        bool                       m_stop = false;
};

#define sReloadMgr ReloadMgr::instance()

#endif
