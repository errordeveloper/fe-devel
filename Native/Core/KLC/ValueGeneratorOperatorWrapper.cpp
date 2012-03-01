/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/ValueGeneratorOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueGeneratorOperator.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Core/AST/Operator.h>
#include <Fabric/Core/AST/Param.h>
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/RT/Impl.h>
#include <Fabric/Core/RT/IntegerDesc.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace KLC
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( ValueGeneratorOperatorWrapper, OperatorWrapper )

    RC::Handle<ValueGeneratorOperatorWrapper> ValueGeneratorOperatorWrapper::Create(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      GenericFunctionPtr functionPtr
      )
    {
      return new ValueGeneratorOperatorWrapper( FABRIC_GC_OBJECT_MY_CLASS, executable, astOperator, functionPtr );
    }

    ValueGeneratorOperatorWrapper::ValueGeneratorOperatorWrapper(
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
        ValueGeneratorOperator::Create(
          executable,
          astOperator,
          functionPtr
          )
        )
    {
    }
       
    char const *ValueGeneratorOperatorWrapper::getKind() const
    {
      return "ValueGeneratorOperator";
    }
        
    RC::ConstHandle<ValueGeneratorOperator> ValueGeneratorOperatorWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }
  }
}
