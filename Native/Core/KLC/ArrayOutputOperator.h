/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KLC_ARRAY_OUTPUT_OPERATOR_H
#define _FABRIC_KLC_ARRAY_OUTPUT_OPERATOR_H

#include <Fabric/Core/MR/ArrayOutputOperator.h>

namespace Fabric
{
  namespace AST
  {
    class Operator;
  }
  
  namespace KLC
  {
    class Executable;

    class ArrayOutputOperator : public MR::ArrayOutputOperator
    {
    public:
      REPORT_RC_LEAKS

    protected:
    
      ArrayOutputOperator(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        void (*functionPtr)(...)
        );
     
    private:
    
      RC::ConstHandle<Executable> m_executable;
    };
  }
}

#endif //_FABRIC_KLC_ARRAY_OUTPUT_OPERATOR_H
