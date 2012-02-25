/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CONST_UNSIGNED_INTEGER_H
#define _FABRIC_AST_CONST_UNSIGNED_INTEGER_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class ConstUnsignedInteger : public Expr
    {
      FABRIC_AST_NODE_DECL( ConstUnsignedInteger );

    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<ConstUnsignedInteger> Create( CG::Location const &location, std::string const &valueString );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
        
    protected:
    
      ConstUnsignedInteger( CG::Location const &location, std::string const &valueString );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
    
      std::string m_valueString;
    };
  }
}

#endif //_FABRIC_AST_CONST_UNSIGNED_INTEGER_H
