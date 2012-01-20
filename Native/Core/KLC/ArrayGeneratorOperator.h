/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_KLC_ARRAY_GENERATOR_OPERATOR_H
#define _FABRIC_KLC_ARRAY_GENERATOR_OPERATOR_H

#include <Fabric/Core/KLC/ArrayOutputOperator.h>

namespace Fabric
{
  namespace AST
  {
    class Operator;
  }
  
  namespace KLC
  {
    class Executable;

    class ArrayGeneratorOperator : public ArrayOutputOperator
    {
    public:
    
      static RC::Handle<ArrayGeneratorOperator> Create(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        void (*functionPtr)(...)
        );
      
    protected:
    
      ArrayGeneratorOperator(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        void (*functionPtr)(...)
        );
    };
  }
}

#endif //_FABRIC_KLC_ARRAY_GENERATOR_OPERATOR_H
