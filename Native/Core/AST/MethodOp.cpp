/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/MethodOp.h>
#include <Fabric/Core/AST/ExprVector.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Core/CG/PencilSymbol.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( MethodOp );
    
    RC::ConstHandle<MethodOp> MethodOp::Create(
      CG::Location const &location,
      std::string const &name,
      RC::ConstHandle<Expr> const &expr,
      RC::ConstHandle<ExprVector> const &args
      )
    {
      return new MethodOp( location, name, expr, args );
    }
    
    MethodOp::MethodOp(
      CG::Location const &location,
      std::string const &name,
      RC::ConstHandle<Expr> const &expr,
      RC::ConstHandle<ExprVector> const &args
      )
      : Expr( location )
      , m_name( name )
      , m_expr( expr )
      , m_args( args )
    {
    }
    
    void MethodOp::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_expr->appendJSON( jsonObjectEncoder.makeMember( "expr" ), includeLocation );
      jsonObjectEncoder.makeMember( "methodName" ).makeString( m_name );
      m_args->appendJSON( jsonObjectEncoder.makeMember( "args" ), includeLocation );
    }
    
    CG::Function const *MethodOp::getFunction( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      CG::ExprType thisType = m_expr->getExprType( basicBlockBuilder );
      
      CG::ExprTypeVector argTypes;
      m_args->appendExprTypes( basicBlockBuilder, argTypes );

      return basicBlockBuilder.getModuleBuilder().getFunction(
        getLocation(),
        CG::MethodPencilKey( thisType.getAdapter(), m_name ),
        CG::ExprTypeVector(
          thisType,
          argTypes
          ),
        CG::MethodQueryDesc(
          thisType,
          m_name,
          argTypes
          )
        );
    }
    
    void MethodOp::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_expr->registerTypes( cgManager, diagnostics );
      m_args->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType MethodOp::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = getFunction( basicBlockBuilder )->getReturnInfo().getAdapter();
      if ( adapter )
      {
        adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
        return CG::ExprType( adapter, CG::USAGE_RVALUE );
      }
      else return CG::ExprType();
    }
    
    CG::ExprValue MethodOp::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      CG::Function const *function = getFunction( basicBlockBuilder );
      CG::ParamVector const &functionParams = function->getParams();
      
      try
      {
        CG::Usage thisUsage = functionParams[0].getUsage();
        std::vector<CG::Usage> argUsages;
        for ( size_t i=1; i<functionParams.size(); ++i )
          argUsages.push_back( functionParams[i].getUsage() );
        
        std::vector<CG::ExprValue> argExprValues;
        CG::ExprValue thisExprValue = m_expr->buildExprValue( basicBlockBuilder, thisUsage, "cannot be modified" );
        argExprValues.push_back( thisExprValue );
        m_args->appendExprValues( basicBlockBuilder, argUsages, argExprValues, "cannot be used as an io argument" );
        
        CG::ExprValue callResultExprValue = function->llvmCreateCall( basicBlockBuilder, argExprValues );

        CG::ExprValue result( basicBlockBuilder.getContext() );
        if ( function->getReturnInfo().getExprType() )
          result = callResultExprValue;
        else result = thisExprValue;

        return result.castTo( basicBlockBuilder, usage );
      }
      catch ( CG::Error e )
      {
        throw "calling method " + _(m_name) + " of type " + _(functionParams[0].getAdapter()->getUserName()) + ": " + e;
      }
      catch ( Exception e )
      {
        throw CG::Error( getLocation(), "calling method " + _(m_name) + " of type " + _(functionParams[0].getAdapter()->getUserName()) + ": " + e );
      }
    }
  };
};
