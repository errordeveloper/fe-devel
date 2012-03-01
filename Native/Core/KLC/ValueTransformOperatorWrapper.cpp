/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/ValueTransformOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueTransformOperator.h>
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
    FABRIC_GC_OBJECT_CLASS_IMPL( ValueTransformOperatorWrapper, OperatorWrapper )

    RC::Handle<ValueTransformOperatorWrapper> ValueTransformOperatorWrapper::Create(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      GenericFunctionPtr functionPtr
      )
    {
      return new ValueTransformOperatorWrapper( FABRIC_GC_OBJECT_MY_CLASS, executable, astOperator, functionPtr );
    }

    ValueTransformOperatorWrapper::ValueTransformOperatorWrapper(
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
        ValueTransformOperator::Create(
          executable,
          astOperator,
          functionPtr
          )
        )
    {
    }
       
    char const *ValueTransformOperatorWrapper::getKind() const
    {
      return "ValueTransformOperator";
    }
        
    RC::ConstHandle<ValueTransformOperator> ValueTransformOperatorWrapper::getUnwrapped() const
    {
      return m_unwrapped;
    }
  }
}
