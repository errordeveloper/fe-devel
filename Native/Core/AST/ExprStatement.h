/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_EXPR_STATEMENT_H
#define _FABRIC_AST_EXPR_STATEMENT_H

#include <Fabric/Core/AST/Statement.h>

namespace Fabric
{
  namespace AST
  {
    class Expr;
    
    class ExprStatement: public Statement
    {
      FABRIC_AST_NODE_DECL( ExprStatement );

    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<ExprStatement> Create( CG::Location const &location, RC::ConstHandle<Expr> const &expr );

      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual void llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;
     
    protected:
    
      ExprStatement( CG::Location const &location, RC::ConstHandle<Expr> const &expr );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      RC::ConstHandle<Expr> m_expr;
    };
  };
};

#endif //_FABRIC_AST_EXPR_STATEMENT_H
