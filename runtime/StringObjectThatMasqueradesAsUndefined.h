
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
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef StringObjectThatMasqueradesAsUndefined_h
#define StringObjectThatMasqueradesAsUndefined_h

#include "AJGlobalObject.h"
#include "StringObject.h"
#include "UString.h"

namespace AJ {

    // WebCore uses this to make style.filter undetectable
    class StringObjectThatMasqueradesAsUndefined : public StringObject {
    public:
        static StringObjectThatMasqueradesAsUndefined* create(ExecState* exec, const UString& string)
        {
            return new (exec) StringObjectThatMasqueradesAsUndefined(exec,
                createStructure(exec->lexicalGlobalObject()->stringPrototype()), string);
        }

    private:
        StringObjectThatMasqueradesAsUndefined(ExecState* exec, NonNullPassRefPtr<Structure> structure, const UString& string)
            : StringObject(exec, structure, string)
        {
        }

        static PassRefPtr<Structure> createStructure(AJValue proto) 
        { 
            return Structure::create(proto, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount); 
        }

        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | MasqueradesAsUndefined | OverridesGetPropertyNames | StringObject::StructureFlags;

        virtual bool toBoolean(ExecState*) const { return false; }
    };
 
} // namespace AJ

#endif // StringObjectThatMasqueradesAsUndefined_h
