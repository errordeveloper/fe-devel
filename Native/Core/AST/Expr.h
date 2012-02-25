/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_EXPR_H
#define _FABRIC_AST_EXPR_H

#include <Fabric/Core/AST/Node.h>
#include <Fabric/Core/CG/ExprValue.h>

namespace Fabric
{
  namespace CG
  {
    class BasicBlockBuilder;
    class Diagnostics;
    class Manager;
    class ModuleBuilder;
  };
  
  namespace AST
  {
    class Expr: public Node
    {
    public:
      REPORT_RC_LEAKS
    
      Expr( CG::Location const &location );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const = 0;
    
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const = 0;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const = 0;
    };
  };
};

#endif //_FABRIC_AST_EXPR_H
