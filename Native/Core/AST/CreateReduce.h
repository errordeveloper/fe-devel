/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CREATE_REDUCE_H
#define _FABRIC_AST_CREATE_REDUCE_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class CreateReduce : public Expr
    {
      FABRIC_AST_NODE_DECL( CreateReduce );

    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<CreateReduce> Create(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &input,
        std::string const &operatorName,
        RC::ConstHandle<Expr> const &shared
        );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
        
    protected:
    
      CreateReduce(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &input,
        std::string const &operatorName,
        RC::ConstHandle<Expr> const &shared
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
    
      RC::ConstHandle<Expr> m_input;
      std::string m_operatorName;
      RC::ConstHandle<Expr> m_shared;
    };
  };
};

#endif //_FABRIC_AST_CREATE_REDUCE_H
