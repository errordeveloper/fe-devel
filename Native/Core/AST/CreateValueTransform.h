/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CREATE_VALUE_TRANSFORM_H
#define _FABRIC_AST_CREATE_VALUE_TRANSFORM_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class CreateValueTransform : public Expr
    {
      FABRIC_AST_NODE_DECL( CreateValueTransform );

    public:
    
      static RC::ConstHandle<CreateValueTransform> Create(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &input,
        std::string const &operatorName,
        RC::ConstHandle<Expr> const &shared
        );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual RC::ConstHandle<CG::Adapter> getType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
        
    protected:
    
      CreateValueTransform(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &input,
        std::string const &operatorName,
        RC::ConstHandle<Expr> const &shared
        );
      
      virtual void appendJSONMembers( Util::JSONObjectGenerator const &jsonObjectGenerator, bool includeLocation ) const;
      
    private:
    
      RC::ConstHandle<Expr> m_input;
      std::string m_operatorName;
      RC::ConstHandle<Expr> m_shared;
    };
  };
};

#endif //_FABRIC_AST_CREATE_VALUE_TRANSFORM_H