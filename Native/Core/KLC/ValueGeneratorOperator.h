/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KLC_VALUE_GENERATOR_OPERATOR_H
#define _FABRIC_KLC_VALUE_GENERATOR_OPERATOR_H

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

    class ValueGeneratorOperator : public ValueOutputOperator
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ValueGeneratorOperator> Create(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
      
    protected:
    
      ValueGeneratorOperator(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
    };
  }
}

#endif //_FABRIC_KLC_VALUE_GENERATOR_OPERATOR_H
