/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/ArrayGeneratorOperatorWrapper.h>
#include <Fabric/Core/KLC/ArrayGeneratorOperator.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/AST/Operator.h>

namespace Fabric
{
  namespace KLC
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ArrayGeneratorOperatorWrapper, OperatorWrapper )

    RC::Handle<ArrayGeneratorOperatorWrapper> ArrayGeneratorOperatorWrapper::Create(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      GenericFunctionPtr functionPtr
      )
    {
      return new ArrayGeneratorOperatorWrapper( FABRIC_GC_OBJECT_MY_CLASS, executable, astOperator, functionPtr );
    }

    ArrayGeneratorOperatorWrapper::ArrayGeneratorOperatorWrapper(
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
        ArrayGeneratorOperator::Create(
          executable,
          astOperator,
          functionPtr
          )
        )
    {
    }
       
    char const *ArrayGeneratorOperatorWrapper::getKind() const
    {
      return "ArrayGeneratorOperator";
    }
        
    RC::ConstHandle<ArrayGeneratorOperator> ArrayGeneratorOperatorWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }
  }
}
