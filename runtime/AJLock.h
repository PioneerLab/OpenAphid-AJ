
/*
Copyright 2012 Aphid Mobile

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
 
   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/
/*
 * Copyright (C) 2005, 2008, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef AJLock_h
#define AJLock_h

#include <wtf/Assertions.h>
#include <wtf/Noncopyable.h>

namespace AJ {

    // To make it safe to use AJ on multiple threads, it is
    // important to lock before doing anything that allocates a
    // AJ data structure or that interacts with shared state
    // such as the protect count hash table. The simplest way to lock
    // is to create a local AJLock object in the scope where the lock 
    // must be held. The lock is recursive so nesting is ok. The AJLock 
    // object also acts as a convenience short-hand for running important
    // initialization routines.

    // To avoid deadlock, sometimes it is necessary to temporarily
    // release the lock. Since it is recursive you actually have to
    // release all locks held by your thread. This is safe to do if
    // you are executing code that doesn't require the lock, and you
    // reacquire the right number of locks at the end. You can do this
    // by constructing a locally scoped AJLock::DropAllLocks object. The 
    // DropAllLocks object takes care to release the AJLock only if your
    // thread acquired it to begin with.

    // For contexts other than the single shared one, implicit locking is not done,
    // but we still need to perform all the counting in order to keep debug
    // assertions working, so that clients that use the shared context don't break.

    class ExecState;

    enum AJLockBehavior { SilenceAssertionsOnly, LockForReal };

    class AJLock : public Noncopyable {
    public:
        AJLock(ExecState*);

        AJLock(AJLockBehavior lockBehavior)
            : m_lockBehavior(lockBehavior)
        {
#ifdef NDEBUG
            // Locking "not for real" is a debug-only feature.
            if (lockBehavior == SilenceAssertionsOnly)
                return;
#endif
            lock(lockBehavior);
        }

        ~AJLock()
        { 
#ifdef NDEBUG
            // Locking "not for real" is a debug-only feature.
            if (m_lockBehavior == SilenceAssertionsOnly)
                return;
#endif
            unlock(m_lockBehavior); 
        }
        
        static void lock(AJLockBehavior);
        static void unlock(AJLockBehavior);
        static void lock(ExecState*);
        static void unlock(ExecState*);

        static intptr_t lockCount();
        static bool currentThreadIsHoldingLock();

        AJLockBehavior m_lockBehavior;

        class DropAllLocks : public Noncopyable {
        public:
            DropAllLocks(ExecState* exec);
            DropAllLocks(AJLockBehavior);
            ~DropAllLocks();
            
        private:
            intptr_t m_lockCount;
            AJLockBehavior m_lockBehavior;
        };
    };

} // namespace

#endif // AJLock_h
