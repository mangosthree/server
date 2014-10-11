/**
 * MaNGOS is a full featured server for World of Warcraft, supporting
 * the following clients: 1.12.x, 2.4.3, 3.3.5a, 4.3.4a and 5.4.8
 *
 * Copyright (C) 2005-2014  MaNGOS project <http://getmangos.eu>
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

#ifndef MANGOS_CREATIONPOLICY_H
#define MANGOS_CREATIONPOLICY_H

#include <stdlib.h>
#include "Platform/Define.h"

namespace MaNGOS
{
    /**
     * OperatorNew policy creates an object on the heap using new.
     */
    template<class T>
    class MANGOS_DLL_DECL OperatorNew
    {
        public:

            static T* Create()
            {
                return (new T);
            }

            static void Destroy(T* obj)
            {
                delete obj;
            }
    };

    /**
     * LocalStaticCreation policy creates an object on the stack
     * the first time call Create.
     */
    template<class T>
    class MANGOS_DLL_DECL LocalStaticCreation
    {
            union MaxAlign
            {
                char t_[sizeof(T)];
                short int shortInt_;
                int int_;
                long int longInt_;
                float float_;
                double double_;
                long double longDouble_;
                struct Test;
                int Test::* pMember_;
                int (Test::*pMemberFn_)(int);
            };

        public:

            static T* Create()
            {
                static MaxAlign si_localStatic;
                return new(&si_localStatic) T;
            }

            static void Destroy(T* obj)
            {
                obj->~T();
            }
    };

    /**
     * CreateUsingMalloc by pass the memory manger.
     */
    template<class T>
    class MANGOS_DLL_DECL CreateUsingMalloc
    {
        public:

            static T* Create()
            {
                void* p = malloc(sizeof(T));

                if (!p)
                    return NULL;

                return new(p) T;
            }

            static void Destroy(T* p)
            {
                p->~T();
                free(p);
            }
    };

    /**
     * CreateOnCallBack creates the object base on the call back.
     */
    template<class T, class CALL_BACK>
    class MANGOS_DLL_DECL CreateOnCallBack
    {
        public:
            static T* Create()
            {
                return CALL_BACK::createCallBack();
            }

            static void Destroy(T* p)
            {
                CALL_BACK::destroyCallBack(p);
            }
    };
}

#endif
