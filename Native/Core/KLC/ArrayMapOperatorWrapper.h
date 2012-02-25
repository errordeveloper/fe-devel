/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KLC_ARRAY_MAP_OPERATOR_WRAPPER_H
#define _FABRIC_KLC_ARRAY_MAP_OPERATOR_WRAPPER_H

#include <Fabric/Core/KLC/OperatorWrapper.h>

namespace Fabric
{
  namespace KLC
  {
    class ArrayMapOperator;
    
    class ArrayMapOperatorWrapper : public OperatorWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
      
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ArrayMapOperatorWrapper> Create(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
      
      RC::ConstHandle<ArrayMapOperator> getUnwrapped() const;
      
    protected:
    
      ArrayMapOperatorWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
       
      virtual char const *getKind() const;
    
    private:
    
      RC::ConstHandle<ArrayMapOperator> m_unwrapped;
    };
  }
}

#endif //_FABRIC_KLC_ARRAY_MAP_OPERATOR_WRAPPER_H
