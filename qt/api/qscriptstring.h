
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
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef qscriptstring_h
#define qscriptstring_h

#include "qtscriptglobal.h"
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>

class QScriptStringPrivate;
typedef QExplicitlySharedDataPointer<QScriptStringPrivate> QScriptStringPtr;

class Q_JAVASCRIPT_EXPORT QScriptString {
public:
    QScriptString();
    QScriptString(const QScriptString& other);
    ~QScriptString();

    QScriptString& operator=(const QScriptString& other);

    bool isValid() const;

    bool operator==(const QScriptString& other) const;
    bool operator!=(const QScriptString& other) const;

    quint32 toArrayIndex(bool* ok = 0) const;

    QString toString() const;
    operator QString() const;

private:
    QScriptString(QScriptStringPrivate* d);

    QScriptStringPtr d_ptr;

    friend class QScriptStringPrivate;
};

uint qHash(const QScriptString& key);

#endif // qscriptstring_h
