/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/ValueMapOperator.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/AST/Operator.h>

namespace Fabric
{
  namespace KLC
  {
    RC::Handle<ValueMapOperator> ValueMapOperator::Create(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      GenericFunctionPtr functionPtr
      )
    {
      return new ValueMapOperator(
        executable,
        astOperator,
        functionPtr
        );
    }

    ValueMapOperator::ValueMapOperator(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      GenericFunctionPtr functionPtr
      )
      : ValueIOOperator(
        executable,
        astOperator,
        functionPtr
        )
    {
    }
  }
}
