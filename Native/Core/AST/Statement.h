/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_STATEMENT_H
#define _FABRIC_AST_STATEMENT_H

#include <Fabric/Core/AST/Node.h>

namespace llvm
{
  class BasicBlock;
}

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace CG
  {
    class BasicBlockBuilder;
    class Diagnostics;
    class Manager;
    class ModuleBuilder;
  };
  
  namespace AST
  {
    class Statement : public Node
    {
    public:
      REPORT_RC_LEAKS
      
      Statement( CG::Location const &location );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const = 0;
      
      virtual void llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const = 0;
    };
  };
};

#endif //_FABRIC_AST_STATEMENT_H
