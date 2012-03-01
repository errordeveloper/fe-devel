/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/ArrayMapOperatorWrapper.h>
#include <Fabric/Core/KLC/ArrayMapOperator.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/AST/Operator.h>

namespace Fabric
{
  namespace KLC
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ArrayMapOperatorWrapper, OperatorWrapper )

    RC::Handle<ArrayMapOperatorWrapper> ArrayMapOperatorWrapper::Create(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      GenericFunctionPtr functionPtr
      )
    {
      return new ArrayMapOperatorWrapper( FABRIC_GC_OBJECT_MY_CLASS, executable, astOperator, functionPtr );
    }

    ArrayMapOperatorWrapper::ArrayMapOperatorWrapper(
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
        ArrayMapOperator::Create(
          executable,
          astOperator,
          functionPtr
          )
        )
    {
    }
       
    char const *ArrayMapOperatorWrapper::getKind() const
    {
      return "ArrayMapOperator";
    }
        
    RC::ConstHandle<ArrayMapOperator> ArrayMapOperatorWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }
  }
}
