/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KLC_VALUE_OUTPUT_OPERATOR_H
#define _FABRIC_KLC_VALUE_OUTPUT_OPERATOR_H

#include <Fabric/Core/MR/ValueOutputOperator.h>
#include <Fabric/Core/KLC/GenericFunctionPtr.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace Fabric
{
  namespace AST
  {
    class Operator;
  }
  
  namespace KLC
  {
    class Executable;

    class ValueOutputOperator : public MR::ValueOutputOperator
    {
    protected:
      REPORT_RC_LEAKS
    
      ValueOutputOperator(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
    
    private:
    
      RC::ConstHandle<Executable> m_executable;
    };
  }
}

#endif //_FABRIC_KLC_VALUE_OUTPUT_OPERATOR_H
