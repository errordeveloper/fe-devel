/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/AST/Call.h>
#include <Fabric/Core/AST/ExprVector.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/ExprValue.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Core/CG/PencilSymbol.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( Call );
    
    RC::ConstHandle<Call> Call::Create(
      CG::Location const &location,
      std::string const &name,
      RC::ConstHandle<ExprVector> const &args
      )
    {
      return new Call( location, name, args );
    }

    Call::Call(
      CG::Location const &location,
      std::string const &name,
      RC::ConstHandle<ExprVector> const &args
      )
      : Expr( location )
      , m_name( name )
      , m_args( args )
    {
    }
    
    void Call::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "functionFriendlyName" ).makeString( m_name );
      m_args->appendJSON( jsonObjectEncoder.makeMember( "args" ), includeLocation );
    }
    
    CG::Function const *Call::getFunction( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Symbol> symbol = basicBlockBuilder.getScope().get( m_name );
      if ( !symbol )
      {
        std::string pencilKey = CG::FunctionPencilKey( m_name );
        throw CG::Error( getLocation(), "no such function " + _(m_name) );
      }
      if ( !symbol->isPencil() )
        throw Exception( _(m_name) + " is not a function" );
        
      CG::ExprTypeVector argExprTypes;
      m_args->appendExprTypes( basicBlockBuilder, argExprTypes );
      
      RC::ConstHandle<CG::PencilSymbol> pencilSymbol = RC::ConstHandle<CG::PencilSymbol>::StaticCast( symbol );
      return pencilSymbol->getFunction(
        getLocation(),
        basicBlockBuilder.getModuleBuilder(),
        argExprTypes,
        CG::FunctionQueryDesc(
          m_name,
          argExprTypes
          )
        );
    }
    
    CG::ExprType Call::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = basicBlockBuilder.maybeGetAdapter( m_name );
      if ( !adapter )
      {
        CG::Function const *function = getFunction( basicBlockBuilder );
        adapter = function->getReturnInfo().getAdapter();
      }
      
      CG::ExprType exprType;
      if ( adapter )
      {
        exprType = CG::ExprType( adapter, CG::USAGE_RVALUE );
        adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      }
      return exprType;
    }
    
    void Call::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_args->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprValue Call::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = basicBlockBuilder.maybeGetAdapter( m_name );
      if ( adapter )
      {
        adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
        
        if ( usage == CG::USAGE_LVALUE )
          throw Exception( "temporary values cannot be used in an l-value context" );
        else usage = CG::USAGE_RVALUE;
        
        llvm::Value *thisLValue = adapter->llvmAlloca( basicBlockBuilder, "temp"+adapter->getUserName() );
        adapter->llvmInit( basicBlockBuilder, thisLValue );
        CG::ExprValue result( adapter, CG::USAGE_LVALUE, basicBlockBuilder.getContext(), thisLValue );
        basicBlockBuilder.getScope().put( CG::VariableSymbol::Create( result ) );
        
        std::vector< RC::ConstHandle<CG::Adapter> > argAdapters;
        m_args->appendAdapters( basicBlockBuilder, argAdapters );
        if ( argAdapters.size() == 1 && argAdapters[0] == adapter )
        {
          std::vector<CG::Usage> usages;
          usages.push_back( CG::USAGE_RVALUE );
          std::vector<CG::ExprValue> exprValues;
          m_args->appendExprValues( basicBlockBuilder, usages, exprValues, "cannot be used as an io argument" );
          FABRIC_ASSERT( exprValues.size() == 1 );
          adapter->llvmAssign( basicBlockBuilder, thisLValue, exprValues[0].getValue() );
        }
        else
        {
          CG::ExprTypeVector argTypes;
          m_args->appendExprTypes( basicBlockBuilder, argTypes );

          CG::Function const *function = basicBlockBuilder.getModuleBuilder().getFunction(
            getLocation(),
            CG::ConstructorPencilKey( result.getAdapter() ),
            CG::ExprTypeVector(
              result.getExprType(),
              argTypes
              ),
            CG::ConstructorQueryDesc(
              result.getAdapter(),
              argTypes.getAdapters()
              )
            );

          CG::ParamVector const functionParams = function->getParams();
          
          std::vector<CG::Usage> argUsages;
          for ( size_t i=1; i<functionParams.size(); ++i )
            argUsages.push_back( functionParams[i].getUsage() );
            
          std::vector<CG::ExprValue> exprValues;
          exprValues.push_back( result );
          m_args->appendExprValues( basicBlockBuilder, argUsages, exprValues, "cannot be used as an io argument" );
          
          function->llvmCreateCall( basicBlockBuilder, exprValues );
        }
        
        result.castTo( basicBlockBuilder, usage );
        return result;
      }
      else
      {
        CG::Function const *function = getFunction( basicBlockBuilder );
        if ( usage == CG::USAGE_LVALUE && function->getReturnInfo().getUsage() != CG::USAGE_LVALUE )
          throw Exception( "result of function "+_(m_name)+" is not an l-value" );
        
        CG::ExprValue result( basicBlockBuilder.getContext() );
        try
        {
          CG::ParamVector const functionParams = function->getParams();

          std::vector<CG::Usage> paramUsages;
          for ( size_t i=0; i<functionParams.size(); ++i )
            paramUsages.push_back( functionParams[i].getUsage() );
            
          if ( paramUsages.size() != m_args->size() )
            throw CG::Error( getLocation(), "incorrect number of arguments: expected "+_(paramUsages.size())+", actual "+_(m_args->size()) );

          std::vector<CG::ExprValue> args;
          m_args->appendExprValues( basicBlockBuilder, paramUsages, args, "cannot be an io argument" );
          
          result = function->llvmCreateCall( basicBlockBuilder, args );
        }
        catch ( CG::Error e )
        {
          throw "calling function " + _(m_name) + ": " + e;
        }
        return result.castTo( basicBlockBuilder, usage );
      }
    }
  };
};
