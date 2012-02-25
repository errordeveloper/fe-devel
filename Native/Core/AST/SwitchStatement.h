/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_SWITCH_STATEMENT_H
#define _FABRIC_AST_SWITCH_STATEMENT_H

#include <Fabric/Core/AST/Statement.h>
#include <Fabric/Base/RC/Handle.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace AST
  {
    class Expr;
    class CaseVector;
    class StatementVector;
    
    class SwitchStatement: public Statement
    {
      FABRIC_AST_NODE_DECL( SwitchStatement );

    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<SwitchStatement> Create(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &expr,
        RC::ConstHandle<CaseVector> const &cases
        );

      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual void llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;
     
    protected:
    
      SwitchStatement(
        CG::Location const &location,
        RC::ConstHandle<Expr> const &expr,
        RC::ConstHandle<CaseVector> const &cases
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      RC::ConstHandle<Expr> m_expr;
      RC::ConstHandle<CaseVector> m_cases;
    };
  };
};

#endif //_FABRIC_AST_SWITCH_STATEMENT_H
