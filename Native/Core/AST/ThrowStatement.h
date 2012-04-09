/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_THROW_STATEMENT_H
#define _FABRIC_AST_THROW_STATEMENT_H

#include <Fabric/Core/AST/Statement.h>
#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace AST
  {
    class ThrowStatement: public Statement
    {
      FABRIC_AST_NODE_DECL( ThrowStatement );

    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<ThrowStatement> Create( CG::Location const &location, RC::ConstHandle<Expr> const &expr )
      {
        return new ThrowStatement( location, expr );
      }

      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual void llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;
     
    protected:
    
      ThrowStatement( CG::Location const &location, RC::ConstHandle<Expr> const &expr);
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      RC::ConstHandle<Expr> m_expr;
    };
  }
}

#endif //_FABRIC_AST_THROW_STATEMENT_H
