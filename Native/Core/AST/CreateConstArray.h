/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CREATE_CONST_ARRAY_H
#define _FABRIC_AST_CREATE_CONST_ARRAY_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class CreateConstArray : public Expr
    {
      FABRIC_AST_NODE_DECL( CreateConstArray );

    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<CreateConstArray> Create(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &child
        );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
        
    protected:
    
      CreateConstArray(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &child
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
    
      RC::ConstHandle<Expr> m_child;
    };
  };
};

#endif //_FABRIC_AST_CREATE_CONST_ARRAY_H
