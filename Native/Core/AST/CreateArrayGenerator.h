/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CREATE_ARRAY_GENERATOR_H
#define _FABRIC_AST_CREATE_ARRAY_GENERATOR_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class CreateArrayGenerator : public Expr
    {
      FABRIC_AST_NODE_DECL( CreateArrayGenerator );

    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<CreateArrayGenerator> Create(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &count,
        std::string const &operatorName,
        RC::ConstHandle<Expr> const &shared
        );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
        
    protected:
    
      CreateArrayGenerator(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &count,
        std::string const &operatorName,
        RC::ConstHandle<Expr> const &shared
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
    
      RC::ConstHandle<Expr> m_count;
      std::string m_operatorName;
      RC::ConstHandle<Expr> m_shared;
    };
  };
};

#endif //_FABRIC_AST_CREATE_ARRAY_GENERATOR_H
