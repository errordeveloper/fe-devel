/*
 *  AssignOp.h
 *  Fabric
 *
 *  Created by Halfdan Ingvarsson on 11-01-10.
 *  Copyright 2011 Fabric Technologies Inc. All rights reserved.
 *
 */

#ifndef _FABRIC_AST_ASSIGN_OP_H
#define _FABRIC_AST_ASSIGN_OP_H

#include <Fabric/Core/AST/Expr.h>
#include <Fabric/Core/CG/OpTypes.h>

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
    class AssignOp : public Expr
    {
      FABRIC_AST_NODE_DECL( AssignOp );
      
    public:
      
      static RC::ConstHandle<AssignOp> Create( CG::Location const &location, CG::AssignOpType assignOpType, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &right );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
     
      virtual RC::ConstHandle<CG::Adapter> getType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
      
    protected:
      
      AssignOp( CG::Location const &location, CG::AssignOpType assignOpType, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &right );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
      
      CG::AssignOpType m_assignOpType;
      RC::ConstHandle<Expr> m_left;
      RC::ConstHandle<Expr> m_right;
    };
  };
};

#endif //_FABRIC_AST_ASSIGN_OP_H
