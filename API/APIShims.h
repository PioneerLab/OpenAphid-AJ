
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
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef APIShims_h
#define APIShims_h

#include "CallFrame.h"
#include "AJLock.h"
#include <wtf/ATFThreadData.h>

namespace AJ {

class APIEntryShimWithoutLock {
protected:
    APIEntryShimWithoutLock(AJGlobalData* globalData, bool registerThread)
        : m_globalData(globalData)
        , m_entryIdentifierTable(wtfThreadData().setCurrentIdentifierTable(globalData->identifierTable))
    {
        if (registerThread)
            globalData->heap.registerThread();
        m_globalData->timeoutChecker.start();
    }

    ~APIEntryShimWithoutLock()
    {
        m_globalData->timeoutChecker.stop();
        wtfThreadData().setCurrentIdentifierTable(m_entryIdentifierTable);
    }

private:
    AJGlobalData* m_globalData;
    IdentifierTable* m_entryIdentifierTable;
};

class APIEntryShim : public APIEntryShimWithoutLock {
public:
    // Normal API entry
    APIEntryShim(ExecState* exec, bool registerThread = true)
        : APIEntryShimWithoutLock(&exec->globalData(), registerThread)
        , m_lock(exec)
    {
    }

    // AJPropertyNameAccumulator only has a globalData.
    APIEntryShim(AJGlobalData* globalData, bool registerThread = true)
        : APIEntryShimWithoutLock(globalData, registerThread)
        , m_lock(globalData->isSharedInstance() ? LockForReal : SilenceAssertionsOnly)
    {
    }

private:
    AJLock m_lock;
};

class APICallbackShim {
public:
    APICallbackShim(ExecState* exec)
        : m_dropAllLocks(exec)
        , m_globalData(&exec->globalData())
    {
        wtfThreadData().resetCurrentIdentifierTable();
    }

    ~APICallbackShim()
    {
        wtfThreadData().setCurrentIdentifierTable(m_globalData->identifierTable);
    }

private:
    AJLock::DropAllLocks m_dropAllLocks;
    AJGlobalData* m_globalData;
};

}

#endif
