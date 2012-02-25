/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KLC_VALUE_TRANSFORM_OPERATOR_WRAPPER_H
#define _FABRIC_KLC_VALUE_TRANSFORM_OPERATOR_WRAPPER_H

#include <Fabric/Core/KLC/OperatorWrapper.h>

namespace Fabric
{
  namespace KLC
  {
    class ValueTransformOperator;
    
    class ValueTransformOperatorWrapper : public OperatorWrapper
    {
      FABRIC_GC_OBJECT_CLASS_DECL()
      
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<ValueTransformOperatorWrapper> Create(
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
      
      RC::ConstHandle<ValueTransformOperator> getUnwrapped() const;
      
    protected:
    
      ValueTransformOperatorWrapper(
        FABRIC_GC_OBJECT_CLASS_PARAM,
        RC::ConstHandle<Executable> const &executable,
        RC::ConstHandle<AST::Operator> const &astOperator,
        GenericFunctionPtr functionPtr
        );
       
      virtual char const *getKind() const;
    
    private:
    
      RC::ConstHandle<ValueTransformOperator> m_unwrapped;
    };
  }
}

#endif //_FABRIC_KLC_VALUE_TRANSFORM_OPERATOR_WRAPPER_H
