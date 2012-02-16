/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#ifndef _FABRIC_AST_OR_OP_H
#define _FABRIC_AST_OR_OP_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace AST
  {
    class OrOp : public Expr
    {
      FABRIC_AST_NODE_DECL( OrOp );

    public:
        
      static RC::ConstHandle<OrOp> Create( CG::Location const &location, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &right )
      {
        return new OrOp( location, left, right );
      }

      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
      
    protected:
    
      OrOp( CG::Location const &location, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &right );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;

    private:
    
      RC::ConstHandle<Expr> m_left;
      RC::ConstHandle<Expr> m_right;
    };
  };
};

#endif //_FABRIC_AST_OR_OP_H
