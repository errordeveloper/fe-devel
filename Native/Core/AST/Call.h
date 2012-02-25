/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CALL_H
#define _FABRIC_AST_CALL_H

#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace CG
  {
    class Function;
    class FunctionParam;
  };
  
  namespace AST
  {
    class ExprVector;
    
    class Call : public Expr
    {
      FABRIC_AST_NODE_DECL( Call );
      
    public:
      REPORT_RC_LEAKS
        
      static RC::ConstHandle<Call> Create(
        CG::Location const &location,
        std::string const &name,
        RC::ConstHandle<ExprVector> const &args
        );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual CG::ExprType getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const;
      virtual CG::ExprValue buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const;
      
    protected:
    
      Call(
        CG::Location const &location,
        std::string const &name,
        RC::ConstHandle<ExprVector> const &args
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
      
      CG::Function const *getFunction( CG::BasicBlockBuilder &basicBlockBuilder ) const;

    private:
    
      std::string m_name;
      RC::ConstHandle<ExprVector> m_args;
    };
  };
};

#endif //_FABRIC_AST_CALL_H
