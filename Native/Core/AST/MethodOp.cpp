#include <Fabric/Core/AST/MethodOp.h>
#include <Fabric/Core/AST/ExprVector.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/OverloadNames.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/Error.h>
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
    
    RC::ConstHandle<CG::FunctionSymbol> MethodOp::getFunctionSymbol( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      CG::ExprType thisExprType = m_expr->getExprType( basicBlockBuilder );
      
      std::vector<CG::ExprType> argExprTypes;
      m_args->appendExprTypes( basicBlockBuilder, argExprTypes );
      
      std::string functionName = CG::methodOverloadName( m_name, thisExprType, argExprTypes );
      RC::ConstHandle<CG::FunctionSymbol> functionSymbol = basicBlockBuilder.maybeGetFunction( functionName );
      if ( !functionSymbol )
      {
        std::string functionDesc = m_name + "(";
        for ( size_t i=0; i<argExprTypes.size(); ++i )
        {
          if ( i > 0 )
            functionDesc += ", ";
          if ( argExprTypes[i].getUsage() == CG::USAGE_LVALUE )
            functionDesc += "io ";
          functionDesc += argExprTypes[i].getUserName();
        }
        functionDesc += ")";
        
        throw CG::Error( getLocation(), "type " + thisExprType.getUserName() + " has no method " + _(functionDesc) );
      }
      return functionSymbol;
    }
    
    void MethodOp::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_expr->registerTypes( cgManager, diagnostics );
      m_args->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType MethodOp::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = getFunctionSymbol( basicBlockBuilder )->getReturnInfo().getAdapter();
      if ( adapter )
      {
        adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
        return CG::ExprType( adapter, CG::USAGE_RVALUE );
      }
      else return CG::ExprType();
    }
    
    CG::ExprValue MethodOp::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      RC::ConstHandle<CG::FunctionSymbol> functionSymbol = getFunctionSymbol( basicBlockBuilder );
      std::vector<CG::FunctionParam> const &functionParams = functionSymbol->getParams();
      
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
        
        CG::ExprValue callResultExprValue = functionSymbol->llvmCreateCall( basicBlockBuilder, argExprValues );

        CG::ExprValue result( basicBlockBuilder.getContext() );
        if ( functionSymbol->getReturnInfo().getExprType() )
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
