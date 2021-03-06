
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


#ifndef CallIdentifier_h
#define CallIdentifier_h

#include <runtime/UString.h>
#include "FastAllocBase.h"

namespace AJ {

    struct CallIdentifier : public FastAllocBase {
        UString m_name;
        UString m_url;
        unsigned m_lineNumber;

        CallIdentifier()
            : m_lineNumber(0)
        {
        }

        CallIdentifier(const UString& name, const UString& url, int lineNumber)
            : m_name(name)
            , m_url(!url.isNull() ? url : "")
            , m_lineNumber(lineNumber)
        {
        }

        inline bool operator==(const CallIdentifier& ci) const { return ci.m_lineNumber == m_lineNumber && ci.m_name == m_name && ci.m_url == m_url; }
        inline bool operator!=(const CallIdentifier& ci) const { return !(*this == ci); }

        struct Hash {
            static unsigned hash(const CallIdentifier& key)
            {
                unsigned hashCodes[3] = {
                    key.m_name.rep()->hash(),
                    key.m_url.rep()->hash(),
                    key.m_lineNumber
                };
                return UString::Rep::computeHash(reinterpret_cast<char*>(hashCodes), sizeof(hashCodes));
            }

            static bool equal(const CallIdentifier& a, const CallIdentifier& b) { return a == b; }
            static const bool safeToCompareToEmptyOrDeleted = true;
        };

        unsigned hash() const { return Hash::hash(*this); }

#ifndef NDEBUG
        operator const char*() const { return c_str(); }
        const char* c_str() const { return m_name.UTF8String().data(); }
#endif
    };

} // namespace AJ

namespace ATF {

    template<> struct DefaultHash<AJ::CallIdentifier> { typedef AJ::CallIdentifier::Hash Hash; };

    template<> struct HashTraits<AJ::CallIdentifier> : GenericHashTraits<AJ::CallIdentifier> {
        static void constructDeletedValue(AJ::CallIdentifier& slot)
        {
            new (&slot) AJ::CallIdentifier(AJ::UString(), AJ::UString(), std::numeric_limits<unsigned>::max());
        }
        static bool isDeletedValue(const AJ::CallIdentifier& value)
        {
            return value.m_name.isNull() && value.m_url.isNull() && value.m_lineNumber == std::numeric_limits<unsigned>::max();
        }
    };

} // namespace ATF

#endif  // CallIdentifier_h

