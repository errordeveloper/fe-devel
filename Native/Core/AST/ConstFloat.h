/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CONST_FLOAT_H
#define _FABRIC_AST_CONST_FLOAT_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class ConstFloat : public Expr
    {
      FABRIC_AST_NODE_DECL( ConstFloat );

    public:
      REPORT_RC_LEAKS
      
      static RC::ConstHandle<ConstFloat> Create( CG::Location const &location, std::string const &valueString );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
            
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
        
    protected:
    
      ConstFloat( CG::Location const &location, std::string const &valueString );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
    private:
    
      std::string m_valueString;
    };
  };
};

#endif //_FABRIC_AST_CONST_FLOAT_H
