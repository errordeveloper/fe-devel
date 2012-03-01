/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/ArrayGeneratorOperator.h>
#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/AST/Operator.h>

namespace Fabric
{
  namespace KLC
  {
    RC::Handle<ArrayGeneratorOperator> ArrayGeneratorOperator::Create(
      RC::ConstHandle<Executable> const &executable,
      RC::ConstHandle<AST::Operator> const &astOperator,
      void (*functionPtr)(...)
      )
    {
      return new ArrayGeneratorOperator(
        executable,
        astOperator,
        functionPtr
        );
    }

    ArrayGeneratorOperator::ArrayGeneratorOperator(
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
