/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_TERNARY_OP_H
#define _FABRIC_AST_TERNARY_OP_H

#include <Fabric/Core/AST/Expr.h>
#include <Fabric/Core/CG/OpTypes.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace AST
  {
    class TernaryOp : public Expr
    {
      FABRIC_AST_NODE_DECL( TernaryOp );

    public:
      REPORT_RC_LEAKS
      
      static RC::ConstHandle<TernaryOp> Create( CG::Location const &location, CG::TernaryOpType opType, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &middle, RC::ConstHandle<Expr> const &right )
      {
        return new TernaryOp( location, opType, left, middle, right );
      }

      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
      
    protected:
      
      TernaryOp( CG::Location const &location, CG::TernaryOpType ternaryOpType, RC::ConstHandle<Expr> const &left, RC::ConstHandle<Expr> const &middle, RC::ConstHandle<Expr> const &right );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
      
      CG::TernaryOpType m_opType;
      RC::ConstHandle<Expr> m_left;
      RC::ConstHandle<Expr> m_middle;
      RC::ConstHandle<Expr> m_right;
    };
  };
};

#endif //_FABRIC_AST_TERNARY_OP_H
