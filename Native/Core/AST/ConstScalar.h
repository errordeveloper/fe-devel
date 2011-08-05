/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#ifndef _FABRIC_AST_CONST_SCALAR_H
#define _FABRIC_AST_CONST_SCALAR_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class ConstScalar : public Expr
    {
      FABRIC_AST_NODE_DECL( ConstScalar );

    public:
      
      static RC::ConstHandle<ConstScalar> Create( CG::Location const &location, std::string const &valueString );
      
      virtual void llvmPrepareModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics ) const;
            
      virtual RC::ConstHandle<CG::Adapter> getType( CG::BasicBlockBuilder const &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
        
    protected:
    
      ConstScalar( CG::Location const &location, double value );
      
      virtual void appendJSONMembers( Util::JSONObjectGenerator const &jsonObjectGenerator ) const;
      
    private:
    
      double m_value;
    };
  };
};

#endif //_FABRIC_AST_CONST_SCALAR_H
