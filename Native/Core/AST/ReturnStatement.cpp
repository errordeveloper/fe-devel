/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#include "ReturnStatement.h"
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( ReturnStatement );
    
    ReturnStatement::ReturnStatement( CG::Location const &location, RC::ConstHandle<Expr> const &expr )
      : Statement( location )
      , m_expr( expr )
    {
    }
    
    void ReturnStatement::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Statement::appendJSONMembers( jsonObjectEncoder, includeLocation );
      if ( m_expr )
        m_expr->appendJSON( jsonObjectEncoder.makeMember( "expr" ), includeLocation );
    }
    
    void ReturnStatement::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      if ( m_expr )
        m_expr->registerTypes( cgManager, diagnostics );
    }

    void ReturnStatement::llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      try
      {
        CG::ReturnInfo const &returnInfo = basicBlockBuilder.getFunctionBuilder().getScope().getReturnInfo();
        if ( basicBlockBuilder->GetInsertBlock()->getTerminator() )
          throw CG::Error( getLocation(), "unreachable code" );
        CG::ExprValue returnExprValue( CG::ExprValue( basicBlockBuilder.getContext() ) );
        if ( m_expr )
        {
          if ( !returnInfo )
            throw CG::Error( getLocation(), "functions with no return types do not return values" );
          returnExprValue = m_expr->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, "cannot be assigned to" );
        }
        else
        {
          if ( returnInfo )
            throw CG::Error( getLocation(), "function must return a value" );
        }
        basicBlockBuilder.getScope().llvmReturn( basicBlockBuilder, returnExprValue );
      }
      catch ( Exception e )
      {
        addError( diagnostics, e );
      }
    }
  };
};
