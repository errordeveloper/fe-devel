/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KLC_VALUE_MAP_OPERATOR_H
#define _FABRIC_KLC_VALUE_MAP_OPERATOR_H

#include <Fabric/Core/KLC/ValueIOOperator.h>

namespace Fabric
{
  namespace AST
  {
    class Operator;
  }
  
  namespace KLC
  {
    class Executable;

    class ValueMapOperator : public ValueIOOperator
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ValueMapOperator> Create(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
      
    protected:
    
      ValueMapOperator(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
    };
  }
}

#endif //_FABRIC_KLC_VALUE_MAP_OPERATOR_H
