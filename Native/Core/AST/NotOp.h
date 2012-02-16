/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#ifndef _FABRIC_AST_NOT_OP_H
#define _FABRIC_AST_NOT_OP_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace AST
  {
    class NotOp : public Expr
    {
      FABRIC_AST_NODE_DECL( NotOp );

    public:
        
      static RC::ConstHandle<NotOp> Create( CG::Location const &location, RC::ConstHandle<Expr> const &child )
      {
        return new NotOp( location, child );
      }

      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
      
    protected:
    
      NotOp( CG::Location const &location, RC::ConstHandle<Expr> const &child );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;

    private:
    
      RC::ConstHandle<Expr> m_child;
    };
  };
};

#endif //_FABRIC_AST_NOT_OP_H
