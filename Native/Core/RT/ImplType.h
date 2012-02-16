/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_RT_IMPL_TYPE_H
#define _FABRIC_RT_IMPL_TYPE_H

#include <Fabric/Base/RC/ConstHandle.h>

#include <stddef.h>

namespace Fabric
{
  namespace RT
  {
    class Impl;
    class Desc;
    
    typedef size_t ImplType;
    
    // Simple types
    static const ImplType DT_BOOLEAN = size_t(1) << 0;
    static const ImplType DT_INTEGER = size_t(1) << 1;
    static const ImplType DT_FLOAT = size_t(1) << 2;
    
    // Non-simple, non-compound types
    static const ImplType DT_STRING = size_t(1) << 8;
    static const ImplType DT_OPAQUE = size_t(1) << 9;
    static const ImplType DT_CONST_STRING = size_t(1) << 10;
    static const ImplType DT_VALUE_PRODUCER = size_t(1) << 11;
    static const ImplType DT_ARRAY_PRODUCER = size_t(1) << 12;
    static const ImplType DT_CONTAINER = size_t(1) << 13;
    
    // Struct type
    static const ImplType DT_STRUCT = size_t(1) << 16;
    
    // Array types
    static const ImplType DT_FIXED_ARRAY = size_t(1) << 24;
    static const ImplType DT_VARIABLE_ARRAY = size_t(1) << 25;
    static const ImplType DT_SLICED_ARRAY = size_t(1) << 26;
    
    // Dict type
    static const ImplType DT_DICT = size_t(1) << 30;

    inline bool isBoolean( ImplType implType ) { return implType & DT_BOOLEAN; }
    inline bool isInteger( ImplType implType ) { return implType & DT_INTEGER; }
    inline bool isFloat( ImplType implType ) { return implType & DT_FLOAT; }
    inline bool isNumeric( ImplType implType ) { return isInteger(implType) || isFloat(implType); }
    inline bool isSimple( ImplType implType ) { return isBoolean(implType) || isNumeric(implType); }
    inline bool isString( ImplType implType ) { return implType & DT_STRING; }
    inline bool isOpaque( ImplType implType ) { return implType & DT_OPAQUE; }
    inline bool isConstString( ImplType implType ) { return implType & DT_CONST_STRING; }
    inline bool isValueProducer( ImplType implType ) { return implType & DT_VALUE_PRODUCER; }
    inline bool isArrayProducer( ImplType implType ) { return implType & DT_ARRAY_PRODUCER; }
    inline bool isContainer( ImplType implType ) { return implType & DT_CONTAINER; }
    inline bool isStruct( ImplType implType ) { return implType & DT_STRUCT; }
    inline bool isFixedArray( ImplType implType ) { return implType & DT_FIXED_ARRAY; }
    inline bool isVariableArray( ImplType implType ) { return implType & DT_VARIABLE_ARRAY; }
    inline bool isSlicedArray( ImplType implType ) { return implType & DT_SLICED_ARRAY; }
    inline bool isArray( ImplType implType ) { return isFixedArray(implType) || isVariableArray(implType) || isSlicedArray(implType); }
    inline bool isDict( ImplType implType ) { return implType & DT_DICT; }
    inline bool isComparable( ImplType implType ) { return isSimple(implType) || isString(implType); }
  };
};

#endif //_FABRIC_RT_IMPL_TYPE_H
