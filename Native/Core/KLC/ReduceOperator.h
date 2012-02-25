/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KLC_REDUCE_OPERATOR_H
#define _FABRIC_KLC_REDUCE_OPERATOR_H

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

    class ReduceOperator : public ArrayIOOperator
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ReduceOperator> Create(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        void (*functionPtr)(...)
        );
      
    protected:
    
      ReduceOperator(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        void (*functionPtr)(...)
        );
    };
  }
}

#endif //_FABRIC_KLC_REDUCE_OPERATOR_H
