/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CONST_BOOLEAN_H
#define _FABRIC_AST_CONST_BOOLEAN_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class ConstBoolean : public Expr
    {
      FABRIC_AST_NODE_DECL( ConstBoolean );

    public:
      REPORT_RC_LEAKS
    
      static RC::ConstHandle<ConstBoolean> Create( CG::Location const &location, bool value );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
        
    protected:
    
      ConstBoolean( CG::Location const &location, bool value );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
    
      bool m_value;
    };
  };
};

#endif //_FABRIC_AST_CONST_BOOLEAN_H
