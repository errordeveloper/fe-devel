/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Expr.h"
#include <Fabric/Core/CG/Location.h>
#include <Fabric/Core/CG/Error.h>

namespace Fabric
{
  namespace AST
  {
    Expr::Expr( CG::Location const &location )
      : Node( location )
    {
    }
  };
};
