/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  CopyargList 2010 Fabric Technologies Inc. All argLists reserved.
 *
 */

#ifndef _FABRIC_AST_INDEX_OP_H
#define _FABRIC_AST_INDEX_OP_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class IndexOp : public Expr
    {
      FABRIC_AST_NODE_DECL( IndexOp );

    public:
        
      static RC::ConstHandle<IndexOp> Create(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &expr,
        RC::ConstHandle<Expr> const &indexExpr
        );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
      
    protected:
    
      IndexOp(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &expr,
        RC::ConstHandle<Expr> const &indexExpr
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;

    private:
    
      RC::ConstHandle<Expr> m_expr;
      RC::ConstHandle<Expr> m_indexExpr;
    };
  };
};

#endif //_FABRIC_AST_INDEX_OP_H
