
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
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef ProfileGenerator_h
#define ProfileGenerator_h

#include "Profile.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace AJ {

    class ExecState;
    class Profile;
    class ProfileNode;
    class UString;
    struct CallIdentifier;    

    class ProfileGenerator : public RefCounted<ProfileGenerator>  {
    public:
        static PassRefPtr<ProfileGenerator> create(const UString& title, ExecState* originatingExec, unsigned uid);

        // Members
        const UString& title() const;
        PassRefPtr<Profile> profile() const { return m_profile; }
        ExecState* originatingGlobalExec() const { return m_originatingGlobalExec; }
        unsigned profileGroup() const { return m_profileGroup; }

        // Collecting
        void willExecute(const CallIdentifier&);
        void didExecute(const CallIdentifier&);

        // Stopping Profiling
        void stopProfiling();

        typedef void (ProfileGenerator::*ProfileFunction)(const CallIdentifier& callIdentifier);

    private:
        ProfileGenerator(const UString& title, ExecState* originatingExec, unsigned uid);
        void addParentForConsoleStart(ExecState*);

        void removeProfileStart();
        void removeProfileEnd();

        RefPtr<Profile> m_profile;
        ExecState* m_originatingGlobalExec;
        unsigned m_profileGroup;
        RefPtr<ProfileNode> m_head;
        RefPtr<ProfileNode> m_currentNode;
    };

} // namespace AJ

#endif // ProfileGenerator_h
