
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
// -*- mode: c++; c-basic-offset: 4 -*-
/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef PutPropertySlot_h
#define PutPropertySlot_h

#include <wtf/Assertions.h>

namespace AJ {
    
    class AJObject;
    class AJFunction;
    
    class PutPropertySlot {
    public:
        enum Type { Uncachable, ExistingProperty, NewProperty };

        PutPropertySlot()
            : m_type(Uncachable)
            , m_base(0)
        {
        }

        void setExistingProperty(AJObject* base, size_t offset)
        {
            m_type = ExistingProperty;
            m_base = base;
            m_offset = offset;
        }

        void setNewProperty(AJObject* base, size_t offset)
        {
            m_type = NewProperty;
            m_base = base;
            m_offset = offset;
        }

        Type type() const { return m_type; }
        AJObject* base() const { return m_base; }

        bool isCacheable() const { return m_type != Uncachable; }
        size_t cachedOffset() const {
            ASSERT(isCacheable());
            return m_offset;
        }
    private:
        Type m_type;
        AJObject* m_base;
        size_t m_offset;
    };

} // namespace AJ

#endif // PutPropertySlot_h
