/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#ifndef _FABRIC_AST_STRUCT_MEMBER_OP_H
#define _FABRIC_AST_STRUCT_MEMBER_OP_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace AST
  {
    class StructMemberOp : public Expr
    {
      FABRIC_AST_NODE_DECL( StructMemberOp );

    public:
        
      static RC::ConstHandle<StructMemberOp> Create( CG::Location const &location, RC::ConstHandle<Expr> const &structExpr, std::string const &memberName )
      {
        return new StructMemberOp( location, structExpr, memberName );
      }

      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual RC::ConstHandle<CG::Adapter> getType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
      
    protected:
    
      StructMemberOp( CG::Location const &location, RC::ConstHandle<Expr> const &structExpr, std::string const &memberName );
      
      virtual void appendJSONMembers( Util::JSONObjectGenerator const &jsonObjectGenerator, bool includeLocation ) const;

    private:
    
      RC::ConstHandle<Expr> m_structExpr;
      std::string m_memberName;
    };
  };
};

#endif //_FABRIC_AST_STRUCT_MEMBER_OP_H
