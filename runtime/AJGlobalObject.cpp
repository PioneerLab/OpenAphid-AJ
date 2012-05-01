
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
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AJGlobalObject.h"

#include "AJCallbackConstructor.h"
#include "AJCallbackFunction.h"
#include "AJCallbackObject.h"

#include "Arguments.h"
#include "ArrayConstructor.h"
#include "ArrayPrototype.h"
#include "BooleanConstructor.h"
#include "BooleanPrototype.h"
#include "CodeBlock.h"
#include "DateConstructor.h"
#include "DatePrototype.h"
#include "ErrorConstructor.h"
#include "ErrorPrototype.h"
#include "FunctionConstructor.h"
#include "FunctionPrototype.h"
#include "GlobalEvalFunction.h"
#include "AJFunction.h"
#include "AJGlobalObjectFunctions.h"
#include "AJLock.h"
#include "JSONObject.h"
#include "Interpreter.h"
#include "MathObject.h"
#include "NativeErrorConstructor.h"
#include "NativeErrorPrototype.h"
#include "NumberConstructor.h"
#include "NumberPrototype.h"
#include "ObjectConstructor.h"
#include "ObjectPrototype.h"
#include "Profiler.h"
#include "PrototypeFunction.h"
#include "RegExpConstructor.h"
#include "RegExpMatchesArray.h"
#include "RegExpObject.h"
#include "RegExpPrototype.h"
#include "ScopeChainMark.h"
#include "StringConstructor.h"
#include "StringPrototype.h"
#include "Debugger.h"

namespace AJ {

ASSERT_CLASS_FITS_IN_CELL(AJGlobalObject);

// Default number of ticks before a timeout check should be done.
static const int initialTickCountThreshold = 255;

// Preferred number of milliseconds between each timeout check
static const int preferredScriptCheckTimeInterval = 1000;

static inline void markIfNeeded(MarkStack& markStack, AJValue v)
{
    if (v)
        markStack.append(v);
}

static inline void markIfNeeded(MarkStack& markStack, const RefPtr<Structure>& s)
{
    if (s)
        markIfNeeded(markStack, s->storedPrototype());
}

AJGlobalObject::~AJGlobalObject()
{
    ASSERT(AJLock::currentThreadIsHoldingLock());

    if (d()->debugger)
        d()->debugger->detach(this);

    Profiler** profiler = Profiler::enabledProfilerReference();
    if (UNLIKELY(*profiler != 0)) {
        (*profiler)->stopProfiling(globalExec(), UString());
    }

    d()->next->d()->prev = d()->prev;
    d()->prev->d()->next = d()->next;
    AJGlobalObject*& headObject = head();
    if (headObject == this)
        headObject = d()->next;
    if (headObject == this)
        headObject = 0;

    HashSet<GlobalCodeBlock*>::const_iterator end = codeBlocks().end();
    for (HashSet<GlobalCodeBlock*>::const_iterator it = codeBlocks().begin(); it != end; ++it)
        (*it)->clearGlobalObject();
        
    RegisterFile& registerFile = globalData()->interpreter->registerFile();
    if (registerFile.clearGlobalObject(this))
        registerFile.setNumGlobals(0);
    d()->destructor(d());
}

void AJGlobalObject::init(AJObject* thisValue)
{
    ASSERT(AJLock::currentThreadIsHoldingLock());

    structure()->disableSpecificFunctionTracking();

    d()->globalData = Heap::heap(this)->globalData();
    d()->globalScopeChain = ScopeChain(this, d()->globalData.get(), this, thisValue);

    AJGlobalObject::globalExec()->init(0, 0, d()->globalScopeChain.node(), CallFrame::noCaller(), 0, 0, 0);

    if (AJGlobalObject*& headObject = head()) {
        d()->prev = headObject;
        d()->next = headObject->d()->next;
        headObject->d()->next->d()->prev = this;
        headObject->d()->next = this;
    } else
        headObject = d()->next = d()->prev = this;

    d()->recursion = 0;
    d()->debugger = 0;

    d()->profileGroup = 0;

    reset(prototype());
}

void AJGlobalObject::put(ExecState* exec, const Identifier& propertyName, AJValue value, PutPropertySlot& slot)
{
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    if (symbolTablePut(propertyName, value))
        return;
    JSVariableObject::put(exec, propertyName, value, slot);
}

void AJGlobalObject::putWithAttributes(ExecState* exec, const Identifier& propertyName, AJValue value, unsigned attributes)
{
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    if (symbolTablePutWithAttributes(propertyName, value, attributes))
        return;

    AJValue valueBefore = getDirect(propertyName);
    PutPropertySlot slot;
    JSVariableObject::put(exec, propertyName, value, slot);
    if (!valueBefore) {
        AJValue valueAfter = getDirect(propertyName);
        if (valueAfter)
            AJObject::putWithAttributes(exec, propertyName, valueAfter, attributes);
    }
}

void AJGlobalObject::defineGetter(ExecState* exec, const Identifier& propertyName, AJObject* getterFunc, unsigned attributes)
{
    PropertySlot slot;
    if (!symbolTableGet(propertyName, slot))
        JSVariableObject::defineGetter(exec, propertyName, getterFunc, attributes);
}

void AJGlobalObject::defineSetter(ExecState* exec, const Identifier& propertyName, AJObject* setterFunc, unsigned attributes)
{
    PropertySlot slot;
    if (!symbolTableGet(propertyName, slot))
        JSVariableObject::defineSetter(exec, propertyName, setterFunc, attributes);
}

static inline AJObject* lastInPrototypeChain(AJObject* object)
{
    AJObject* o = object;
    while (o->prototype().isObject())
        o = asObject(o->prototype());
    return o;
}

void AJGlobalObject::reset(AJValue prototype)
{
    ExecState* exec = AJGlobalObject::globalExec();

    // Prototypes

    d()->functionPrototype = new (exec) FunctionPrototype(exec, FunctionPrototype::createStructure(jsNull())); // The real prototype will be set once ObjectPrototype is created.
    d()->prototypeFunctionStructure = PrototypeFunction::createStructure(d()->functionPrototype);
    NativeFunctionWrapper* callFunction = 0;
    NativeFunctionWrapper* applyFunction = 0;
    d()->functionPrototype->addFunctionProperties(exec, d()->prototypeFunctionStructure.get(), &callFunction, &applyFunction);
    d()->callFunction = callFunction;
    d()->applyFunction = applyFunction;
    d()->objectPrototype = new (exec) ObjectPrototype(exec, ObjectPrototype::createStructure(jsNull()), d()->prototypeFunctionStructure.get());
    d()->functionPrototype->structure()->setPrototypeWithoutTransition(d()->objectPrototype);

    d()->emptyObjectStructure = d()->objectPrototype->inheritorID();

    d()->functionStructure = AJFunction::createStructure(d()->functionPrototype);
    d()->callbackFunctionStructure = AJCallbackFunction::createStructure(d()->functionPrototype);
    d()->argumentsStructure = Arguments::createStructure(d()->objectPrototype);
    d()->callbackConstructorStructure = AJCallbackConstructor::createStructure(d()->objectPrototype);
    d()->callbackObjectStructure = AJCallbackObject<AJObject>::createStructure(d()->objectPrototype);

    d()->arrayPrototype = new (exec) ArrayPrototype(ArrayPrototype::createStructure(d()->objectPrototype));
    d()->arrayStructure = AJArray::createStructure(d()->arrayPrototype);
    d()->regExpMatchesArrayStructure = RegExpMatchesArray::createStructure(d()->arrayPrototype);

    d()->stringPrototype = new (exec) StringPrototype(exec, StringPrototype::createStructure(d()->objectPrototype));
    d()->stringObjectStructure = StringObject::createStructure(d()->stringPrototype);

    d()->booleanPrototype = new (exec) BooleanPrototype(exec, BooleanPrototype::createStructure(d()->objectPrototype), d()->prototypeFunctionStructure.get());
    d()->booleanObjectStructure = BooleanObject::createStructure(d()->booleanPrototype);

    d()->numberPrototype = new (exec) NumberPrototype(exec, NumberPrototype::createStructure(d()->objectPrototype), d()->prototypeFunctionStructure.get());
    d()->numberObjectStructure = NumberObject::createStructure(d()->numberPrototype);

    d()->datePrototype = new (exec) DatePrototype(exec, DatePrototype::createStructure(d()->objectPrototype));
    d()->dateStructure = DateInstance::createStructure(d()->datePrototype);

    d()->regExpPrototype = new (exec) RegExpPrototype(exec, RegExpPrototype::createStructure(d()->objectPrototype), d()->prototypeFunctionStructure.get());
    d()->regExpStructure = RegExpObject::createStructure(d()->regExpPrototype);

    d()->methodCallDummy = constructEmptyObject(exec);

    ErrorPrototype* errorPrototype = new (exec) ErrorPrototype(exec, ErrorPrototype::createStructure(d()->objectPrototype), d()->prototypeFunctionStructure.get());
    d()->errorStructure = ErrorInstance::createStructure(errorPrototype);

    // Constructors

    AJCell* objectConstructor = new (exec) ObjectConstructor(exec, ObjectConstructor::createStructure(d()->functionPrototype), d()->objectPrototype, d()->prototypeFunctionStructure.get());
    AJCell* functionConstructor = new (exec) FunctionConstructor(exec, FunctionConstructor::createStructure(d()->functionPrototype), d()->functionPrototype);
    AJCell* arrayConstructor = new (exec) ArrayConstructor(exec, ArrayConstructor::createStructure(d()->functionPrototype), d()->arrayPrototype, d()->prototypeFunctionStructure.get());
    AJCell* stringConstructor = new (exec) StringConstructor(exec, StringConstructor::createStructure(d()->functionPrototype), d()->prototypeFunctionStructure.get(), d()->stringPrototype);
    AJCell* booleanConstructor = new (exec) BooleanConstructor(exec, BooleanConstructor::createStructure(d()->functionPrototype), d()->booleanPrototype);
    AJCell* numberConstructor = new (exec) NumberConstructor(exec, NumberConstructor::createStructure(d()->functionPrototype), d()->numberPrototype);
    AJCell* dateConstructor = new (exec) DateConstructor(exec, DateConstructor::createStructure(d()->functionPrototype), d()->prototypeFunctionStructure.get(), d()->datePrototype);

    d()->regExpConstructor = new (exec) RegExpConstructor(exec, RegExpConstructor::createStructure(d()->functionPrototype), d()->regExpPrototype);

    d()->errorConstructor = new (exec) ErrorConstructor(exec, ErrorConstructor::createStructure(d()->functionPrototype), errorPrototype);

    RefPtr<Structure> nativeErrorPrototypeStructure = NativeErrorPrototype::createStructure(errorPrototype);
    RefPtr<Structure> nativeErrorStructure = NativeErrorConstructor::createStructure(d()->functionPrototype);

    d()->evalErrorConstructor = new (exec) NativeErrorConstructor(exec, nativeErrorStructure, nativeErrorPrototypeStructure, "EvalError");
    d()->rangeErrorConstructor = new (exec) NativeErrorConstructor(exec, nativeErrorStructure, nativeErrorPrototypeStructure, "RangeError");
    d()->referenceErrorConstructor = new (exec) NativeErrorConstructor(exec, nativeErrorStructure, nativeErrorPrototypeStructure, "ReferenceError");
    d()->syntaxErrorConstructor = new (exec) NativeErrorConstructor(exec, nativeErrorStructure, nativeErrorPrototypeStructure, "SyntaxError");
    d()->typeErrorConstructor = new (exec) NativeErrorConstructor(exec, nativeErrorStructure, nativeErrorPrototypeStructure, "TypeError");
    d()->URIErrorConstructor = new (exec) NativeErrorConstructor(exec, nativeErrorStructure, nativeErrorPrototypeStructure, "URIError");

    d()->objectPrototype->putDirectFunctionWithoutTransition(exec->propertyNames().constructor, objectConstructor, DontEnum);
    d()->functionPrototype->putDirectFunctionWithoutTransition(exec->propertyNames().constructor, functionConstructor, DontEnum);
    d()->arrayPrototype->putDirectFunctionWithoutTransition(exec->propertyNames().constructor, arrayConstructor, DontEnum);
    d()->booleanPrototype->putDirectFunctionWithoutTransition(exec->propertyNames().constructor, booleanConstructor, DontEnum);
    d()->stringPrototype->putDirectFunctionWithoutTransition(exec->propertyNames().constructor, stringConstructor, DontEnum);
    d()->numberPrototype->putDirectFunctionWithoutTransition(exec->propertyNames().constructor, numberConstructor, DontEnum);
    d()->datePrototype->putDirectFunctionWithoutTransition(exec->propertyNames().constructor, dateConstructor, DontEnum);
    d()->regExpPrototype->putDirectFunctionWithoutTransition(exec->propertyNames().constructor, d()->regExpConstructor, DontEnum);
    errorPrototype->putDirectFunctionWithoutTransition(exec->propertyNames().constructor, d()->errorConstructor, DontEnum);

    // Set global constructors

    // FIXME: These properties could be handled by a static hash table.

    putDirectFunctionWithoutTransition(Identifier(exec, "Object"), objectConstructor, DontEnum);
    putDirectFunctionWithoutTransition(Identifier(exec, "Function"), functionConstructor, DontEnum);
    putDirectFunctionWithoutTransition(Identifier(exec, "Array"), arrayConstructor, DontEnum);
    putDirectFunctionWithoutTransition(Identifier(exec, "Boolean"), booleanConstructor, DontEnum);
    putDirectFunctionWithoutTransition(Identifier(exec, "String"), stringConstructor, DontEnum);
    putDirectFunctionWithoutTransition(Identifier(exec, "Number"), numberConstructor, DontEnum);
    putDirectFunctionWithoutTransition(Identifier(exec, "Date"), dateConstructor, DontEnum);
    putDirectFunctionWithoutTransition(Identifier(exec, "RegExp"), d()->regExpConstructor, DontEnum);
    putDirectFunctionWithoutTransition(Identifier(exec, "Error"), d()->errorConstructor, DontEnum);
    putDirectFunctionWithoutTransition(Identifier(exec, "EvalError"), d()->evalErrorConstructor);
    putDirectFunctionWithoutTransition(Identifier(exec, "RangeError"), d()->rangeErrorConstructor);
    putDirectFunctionWithoutTransition(Identifier(exec, "ReferenceError"), d()->referenceErrorConstructor);
    putDirectFunctionWithoutTransition(Identifier(exec, "SyntaxError"), d()->syntaxErrorConstructor);
    putDirectFunctionWithoutTransition(Identifier(exec, "TypeError"), d()->typeErrorConstructor);
    putDirectFunctionWithoutTransition(Identifier(exec, "URIError"), d()->URIErrorConstructor);

    // Set global values.
    GlobalPropertyInfo staticGlobals[] = {
        GlobalPropertyInfo(Identifier(exec, "Math"), new (exec) MathObject(exec, MathObject::createStructure(d()->objectPrototype)), DontEnum | DontDelete),
        GlobalPropertyInfo(Identifier(exec, "NaN"), jsNaN(exec), DontEnum | DontDelete | ReadOnly),
        GlobalPropertyInfo(Identifier(exec, "Infinity"), jsNumber(exec, Inf), DontEnum | DontDelete | ReadOnly),
        GlobalPropertyInfo(Identifier(exec, "undefined"), jsUndefined(), DontEnum | DontDelete | ReadOnly),
        GlobalPropertyInfo(Identifier(exec, "JSON"), new (exec) JSONObject(JSONObject::createStructure(d()->objectPrototype)), DontEnum | DontDelete)
    };

    addStaticGlobals(staticGlobals, sizeof(staticGlobals) / sizeof(GlobalPropertyInfo));

    // Set global functions.

    d()->evalFunction = new (exec) GlobalEvalFunction(exec, GlobalEvalFunction::createStructure(d()->functionPrototype), 1, exec->propertyNames().eval, globalFuncEval, this);
    putDirectFunctionWithoutTransition(exec, d()->evalFunction, DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, d()->prototypeFunctionStructure.get(), 2, Identifier(exec, "parseInt"), globalFuncParseInt), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, d()->prototypeFunctionStructure.get(), 1, Identifier(exec, "parseFloat"), globalFuncParseFloat), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, d()->prototypeFunctionStructure.get(), 1, Identifier(exec, "isNaN"), globalFuncIsNaN), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, d()->prototypeFunctionStructure.get(), 1, Identifier(exec, "isFinite"), globalFuncIsFinite), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, d()->prototypeFunctionStructure.get(), 1, Identifier(exec, "escape"), globalFuncEscape), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, d()->prototypeFunctionStructure.get(), 1, Identifier(exec, "unescape"), globalFuncUnescape), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, d()->prototypeFunctionStructure.get(), 1, Identifier(exec, "decodeURI"), globalFuncDecodeURI), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, d()->prototypeFunctionStructure.get(), 1, Identifier(exec, "decodeURIComponent"), globalFuncDecodeURIComponent), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, d()->prototypeFunctionStructure.get(), 1, Identifier(exec, "encodeURI"), globalFuncEncodeURI), DontEnum);
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, d()->prototypeFunctionStructure.get(), 1, Identifier(exec, "encodeURIComponent"), globalFuncEncodeURIComponent), DontEnum);
#ifndef NDEBUG
    putDirectFunctionWithoutTransition(exec, new (exec) NativeFunctionWrapper(exec, d()->prototypeFunctionStructure.get(), 1, Identifier(exec, "jscprint"), globalFuncJSCPrint), DontEnum);
#endif

    resetPrototype(prototype);
}

// Set prototype, and also insert the object prototype at the end of the chain.
void AJGlobalObject::resetPrototype(AJValue prototype)
{
    setPrototype(prototype);

    AJObject* oldLastInPrototypeChain = lastInPrototypeChain(this);
    AJObject* objectPrototype = d()->objectPrototype;
    if (oldLastInPrototypeChain != objectPrototype)
        oldLastInPrototypeChain->setPrototype(objectPrototype);
}

void AJGlobalObject::markChildren(MarkStack& markStack)
{
    JSVariableObject::markChildren(markStack);
    
    HashSet<GlobalCodeBlock*>::const_iterator end = codeBlocks().end();
    for (HashSet<GlobalCodeBlock*>::const_iterator it = codeBlocks().begin(); it != end; ++it)
        (*it)->markAggregate(markStack);

    RegisterFile& registerFile = globalData()->interpreter->registerFile();
    if (registerFile.globalObject() == this)
        registerFile.markGlobals(markStack, &globalData()->heap);

    markIfNeeded(markStack, d()->regExpConstructor);
    markIfNeeded(markStack, d()->errorConstructor);
    markIfNeeded(markStack, d()->evalErrorConstructor);
    markIfNeeded(markStack, d()->rangeErrorConstructor);
    markIfNeeded(markStack, d()->referenceErrorConstructor);
    markIfNeeded(markStack, d()->syntaxErrorConstructor);
    markIfNeeded(markStack, d()->typeErrorConstructor);
    markIfNeeded(markStack, d()->URIErrorConstructor);

    markIfNeeded(markStack, d()->evalFunction);
    markIfNeeded(markStack, d()->callFunction);
    markIfNeeded(markStack, d()->applyFunction);

    markIfNeeded(markStack, d()->objectPrototype);
    markIfNeeded(markStack, d()->functionPrototype);
    markIfNeeded(markStack, d()->arrayPrototype);
    markIfNeeded(markStack, d()->booleanPrototype);
    markIfNeeded(markStack, d()->stringPrototype);
    markIfNeeded(markStack, d()->numberPrototype);
    markIfNeeded(markStack, d()->datePrototype);
    markIfNeeded(markStack, d()->regExpPrototype);

    markIfNeeded(markStack, d()->methodCallDummy);

    markIfNeeded(markStack, d()->errorStructure);
    markIfNeeded(markStack, d()->argumentsStructure);
    markIfNeeded(markStack, d()->arrayStructure);
    markIfNeeded(markStack, d()->booleanObjectStructure);
    markIfNeeded(markStack, d()->callbackConstructorStructure);
    markIfNeeded(markStack, d()->callbackFunctionStructure);
    markIfNeeded(markStack, d()->callbackObjectStructure);
    markIfNeeded(markStack, d()->dateStructure);
    markIfNeeded(markStack, d()->emptyObjectStructure);
    markIfNeeded(markStack, d()->errorStructure);
    markIfNeeded(markStack, d()->functionStructure);
    markIfNeeded(markStack, d()->numberObjectStructure);
    markIfNeeded(markStack, d()->prototypeFunctionStructure);
    markIfNeeded(markStack, d()->regExpMatchesArrayStructure);
    markIfNeeded(markStack, d()->regExpStructure);
    markIfNeeded(markStack, d()->stringObjectStructure);

    // No need to mark the other structures, because their prototypes are all
    // guaranteed to be referenced elsewhere.

    Register* registerArray = d()->registerArray.get();
    if (!registerArray)
        return;

    size_t size = d()->registerArraySize;
    markStack.appendValues(reinterpret_cast<AJValue*>(registerArray), size);
}

ExecState* AJGlobalObject::globalExec()
{
    return CallFrame::create(d()->globalCallFrame + RegisterFile::CallFrameHeaderSize);
}

bool AJGlobalObject::isDynamicScope(bool&) const
{
    return true;
}

void AJGlobalObject::copyGlobalsFrom(RegisterFile& registerFile)
{
    ASSERT(!d()->registerArray);
    ASSERT(!d()->registerArraySize);

    int numGlobals = registerFile.numGlobals();
    if (!numGlobals) {
        d()->registers = 0;
        return;
    }
    
    Register* registerArray = copyRegisterArray(registerFile.lastGlobal(), numGlobals);
    setRegisters(registerArray + numGlobals, registerArray, numGlobals);
}

void AJGlobalObject::copyGlobalsTo(RegisterFile& registerFile)
{
    AJGlobalObject* lastGlobalObject = registerFile.globalObject();
    if (lastGlobalObject && lastGlobalObject != this)
        lastGlobalObject->copyGlobalsFrom(registerFile);

    registerFile.setGlobalObject(this);
    registerFile.setNumGlobals(symbolTable().size());

    if (d()->registerArray) {
        memcpy(registerFile.start() - d()->registerArraySize, d()->registerArray.get(), d()->registerArraySize * sizeof(Register));
        setRegisters(registerFile.start(), 0, 0);
    }
}

void* AJGlobalObject::operator new(size_t size, AJGlobalData* globalData)
{
    return globalData->heap.allocate(size);
}

void AJGlobalObject::destroyAJGlobalObjectData(void* jsGlobalObjectData)
{
    delete static_cast<AJGlobalObjectData*>(jsGlobalObjectData);
}

DynamicGlobalObjectScope::DynamicGlobalObjectScope(CallFrame* callFrame, AJGlobalObject* dynamicGlobalObject)
    : m_dynamicGlobalObjectSlot(callFrame->globalData().dynamicGlobalObject)
    , m_savedDynamicGlobalObject(m_dynamicGlobalObjectSlot)
{
    if (!m_dynamicGlobalObjectSlot) {
#if ENABLE(JIT)
        if (ExecutablePool::underMemoryPressure())
            callFrame->globalData().recompileAllAJFunctions();
#endif
        m_dynamicGlobalObjectSlot = dynamicGlobalObject;

        // Reset the date cache between JS invocations to force the VM
        // to observe time zone changes.
        callFrame->globalData().resetDateCache();
    }
}

} // namespace AJ
