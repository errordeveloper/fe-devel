/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KLC_VALUE_IO_OPERATOR_H
#define _FABRIC_KLC_VALUE_IO_OPERATOR_H

#include <Fabric/Core/MR/ValueIOOperator.h>
#include <Fabric/Core/KLC/GenericFunctionPtr.h>

namespace Fabric
{
  namespace AST
  {
    class Operator;
  }
  
  namespace KLC
  {
    class Executable;

    class ValueIOOperator : public MR::ValueIOOperator
    {
    public:
      REPORT_RC_LEAKS

    protected:
    
      ValueIOOperator(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
    
    private:
    
      RC::ConstHandle<Executable> m_executable;
    };
  }
}

#endif //_FABRIC_KLC_VALUE_IO_OPERATOR_H
