/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_VAR_H
#define _FABRIC_AST_VAR_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace CG
  {
    class ValueSymbol;
  };
  
  namespace AST
  {
    class Var : public Expr
    {
      FABRIC_AST_NODE_DECL( Var );

    public:
    
      static RC::ConstHandle<Var> Create( CG::Location const &location, std::string const &name )
      {
        return new Var( location, name );
      }

      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
        
    protected:
    
      Var( CG::Location const &location, std::string const &name );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
      RC::ConstHandle<CG::ValueSymbol> getValueSymbol( CG::BasicBlockBuilder const &basicBlockBuilder ) const;
      
    private:
    
      std::string m_name;
    };
  };
};

#endif //_FABRIC_AST_VAR_H
