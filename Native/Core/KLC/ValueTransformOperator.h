/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_KLC_VALUE_TRANSFORM_OPERATOR_H
#define _FABRIC_KLC_VALUE_TRANSFORM_OPERATOR_H

#include <Fabric/Core/KLC/ValueOutputOperator.h>

namespace Fabric
{
  namespace AST
  {
    class Operator;
  }
  
  namespace KLC
  {
    class Executable;

    class ValueTransformOperator : public ValueOutputOperator
    {
    public:
    
      static RC::Handle<ValueTransformOperator> Create(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
      
    protected:
    
      ValueTransformOperator(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
    };
  }
}

#endif //_FABRIC_KLC_VALUE_TRANSFORM_OPERATOR_H
