/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_KLC_ARRAY_MAP_OPERATOR_H
#define _FABRIC_KLC_ARRAY_MAP_OPERATOR_H

#include <Fabric/Core/KLC/ArrayIOOperator.h>

namespace Fabric
{
  namespace AST
  {
    class Operator;
  }
  
  namespace KLC
  {
    class Executable;

    class ArrayMapOperator : public ArrayIOOperator
    {
    public:
    
      static RC::Handle<ArrayMapOperator> Create(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        void (*functionPtr)(...)
        );
      
    protected:
    
      ArrayMapOperator(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        void (*functionPtr)(...)
        );
    };
  }
}

#endif //_FABRIC_KLC_ARRAY_MAP_OPERATOR_H
