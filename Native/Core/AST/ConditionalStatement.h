/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CONDITIONAL_STATEMENT_H
#define _FABRIC_AST_CONDITIONAL_STATEMENT_H

#include <Fabric/Core/AST/Statement.h>

namespace Fabric
{
  namespace AST
  {
    class Expr;
    
    class ConditionalStatement: public Statement
    {
      FABRIC_AST_NODE_DECL( ConditionalStatement );

    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<ConditionalStatement> Create(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &expr,
        RC::ConstHandle<Statement> const &trueStatement,
        RC::ConstHandle<Statement> const &falseStatement = RC::ConstHandle<Statement>()
        );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual void llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;
     
    protected:
    
      ConditionalStatement(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &expr,
        RC::ConstHandle<Statement> const &trueStatement,
        RC::ConstHandle<Statement> const &falseStatement = RC::ConstHandle<Statement>()
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      RC::ConstHandle<Expr> m_expr;
      RC::ConstHandle<Statement> m_trueStatement;
      RC::ConstHandle<Statement> m_falseStatement;
    };
  };
};

#endif //_FABRIC_AST_CONDITIONAL_STATEMENT_H
