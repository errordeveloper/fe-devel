/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CONST_STRING_H
#define _FABRIC_AST_CONST_STRING_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace CG
  {
    class ConstStringAdapter;
  };
  
  namespace AST
  {
    class ConstString : public Expr
    {
      FABRIC_AST_NODE_DECL( ConstString );

    public:
    
      static RC::ConstHandle<ConstString> Create( CG::Location const &location, std::string const &value );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
        
    protected:
    
      ConstString( CG::Location const &location, std::string const &value );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
    
      std::string m_value;
    };
  };
};

#endif //_FABRIC_AST_CONST_STRING_H
