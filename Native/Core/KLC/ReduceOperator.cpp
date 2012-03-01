/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/ReduceOperator.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/AST/Operator.h>

namespace Fabric
{
  namespace KLC
  {
    RC::Handle<ReduceOperator> ReduceOperator::Create(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      void (*functionPtr)(...)
      )
    {
      return new ReduceOperator(
        executable,
        astOperator,
        functionPtr
        );
    }

    ReduceOperator::ReduceOperator(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      void (*functionPtr)(...)
      )
      : ArrayIOOperator(
        executable,
        astOperator,
        functionPtr
        )
    {
    }
  }
}
