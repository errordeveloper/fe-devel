/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/ArrayTransformOperatorWrapper.h>
#include <Fabric/Core/KLC/ArrayTransformOperator.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/AST/Operator.h>

namespace Fabric
{
  namespace KLC
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ArrayTransformOperatorWrapper, OperatorWrapper )

    RC::Handle<ArrayTransformOperatorWrapper> ArrayTransformOperatorWrapper::Create(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      GenericFunctionPtr functionPtr
      )
    {
      return new ArrayTransformOperatorWrapper( FABRIC_GC_OBJECT_MY_CLASS, executable, astOperator, functionPtr );
    }

    ArrayTransformOperatorWrapper::ArrayTransformOperatorWrapper(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      GenericFunctionPtr functionPtr
      )
      : OperatorWrapper(
        FABRIC_GC_OBJECT_CLASS_ARG,
        executable,
        astOperator,
        functionPtr
        )
      , m_unwrapped(
        ArrayTransformOperator::Create(
          executable,
          astOperator,
          functionPtr
          )
        )
    {
    }
       
    char const *ArrayTransformOperatorWrapper::getKind() const
    {
      return "ArrayTransformOperator";
    }
        
    RC::ConstHandle<ArrayTransformOperator> ArrayTransformOperatorWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }
  }
}
