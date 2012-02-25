/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_BREAK_STATEMENT_H
#define _FABRIC_AST_BREAK_STATEMENT_H

#include <Fabric/Core/AST/Statement.h>

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
    class BreakStatement: public Statement
    {
      FABRIC_AST_NODE_DECL( BreakStatement );
      
    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<BreakStatement> Create( CG::Location const &location );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual void llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;
     
    protected:
    
      BreakStatement( CG::Location const &location );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    };
  };
};

#endif //_FABRIC_AST_BREAK_STATEMENT_H
