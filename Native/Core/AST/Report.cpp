/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Report.h"
#include <Fabric/Core/CG/StringAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( Report );
    
    Report::Report( CG::Location const &location, RC::ConstHandle<Expr> const &expr )
      : Statement( location )
      , m_expr( expr )
    {
    }
    
    void Report::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Statement::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_expr->appendJSON( jsonObjectEncoder.makeMember( "expr" ), includeLocation );
    }
    
    void Report::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_expr->registerTypes( cgManager, diagnostics );
    }

    void Report::llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      RC::ConstHandle< CG::StringAdapter > stringAdapter = basicBlockBuilder.getManager()->getStringAdapter();
      stringAdapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );

      try
      {
        CG::Scope subScope( basicBlockBuilder.getScope() );
        CG::BasicBlockBuilder subBBB( basicBlockBuilder, subScope );
        CG::ExprValue exprExprValue = m_expr->buildExprValue( subBBB, CG::USAGE_RVALUE, "cannot be an l-value" );
        llvm::Value *stringRValue = stringAdapter->llvmCast( subBBB, exprExprValue );
        stringAdapter->llvmReport( subBBB, stringRValue );
        subScope.llvmUnwind( subBBB );
      }
      catch ( CG::Error e )
      {
        addError( diagnostics, e );
      }
      catch ( Exception e )
      {
        addError( diagnostics, e );
      }
    }
  };
};
