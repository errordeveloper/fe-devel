/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/ArrayTransformOperator.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/AST/Operator.h>

namespace Fabric
{
  namespace KLC
  {
    RC::Handle<ArrayTransformOperator> ArrayTransformOperator::Create(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      void (*functionPtr)(...)
      )
    {
      return new ArrayTransformOperator(
        executable,
        astOperator,
        functionPtr
        );
    }

    ArrayTransformOperator::ArrayTransformOperator(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      void (*functionPtr)(...)
      )
      : ArrayOutputOperator(
        executable,
        astOperator,
        functionPtr
        )
    {
    }
  }
}
