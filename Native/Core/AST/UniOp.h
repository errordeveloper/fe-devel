/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#ifndef _FABRIC_AST_UNI_OP_H
#define _FABRIC_AST_UNI_OP_H

#include <Fabric/Core/AST/Expr.h>
#include <Fabric/Core/CG/OpTypes.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace CG
  {
    class Function;
  };
  
  namespace AST
  {
    class UniOp : public Expr
    {
      FABRIC_AST_NODE_DECL( UniOp );

    public:
    
      static RC::ConstHandle<UniOp> Create( CG::Location const &location, CG::UniOpType uniOpType, RC::ConstHandle<Expr> const &child );

      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
        
    protected:
    
      UniOp( CG::Location const &location, CG::UniOpType uniOpType, RC::ConstHandle<Expr> const &child );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
      CG::Function const *getFunction( CG::BasicBlockBuilder &basicBlockBuilder ) const;

    private:
    
      CG::UniOpType m_uniOpType;
      RC::ConstHandle<Expr> m_child;
    };
  }
}

#endif //_FABRIC_AST_UNI_OP_H
