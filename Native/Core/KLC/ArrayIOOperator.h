/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KLC_ARRAY_IO_OPERATOR_H
#define _FABRIC_KLC_ARRAY_IO_OPERATOR_H

#include <Fabric/Core/MR/ArrayIOOperator.h>

namespace Fabric
{
  namespace AST
  {
    class Operator;
  }
  
  namespace KLC
  {
    class Executable;

    class ArrayIOOperator : public MR::ArrayIOOperator
    {
    protected:
      REPORT_RC_LEAKS
    
      ArrayIOOperator(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        void (*functionPtr)(...)
        );
     
    private:
    
      RC::ConstHandle<Executable> m_executable;
    };
  }
}

#endif //_FABRIC_KLC_ARRAY_IO_OPERATOR_H
