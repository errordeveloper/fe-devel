/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/CG/Function.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Core/CG/PencilSymbol.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/Symbol.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Core/RT/Impl.h>

#include <llvm/Function.h>

namespace Fabric
{
  namespace CG
  {
    bool Function::isExactMatch( ExprTypeVector const &argTypes ) const
    {
      if ( argTypes.size() != m_params.size() )
        return false;
      
      for ( size_t i=0; i<argTypes.size(); ++i )
      {
        Usage argUsage = argTypes[i].getUsage(), paramUsage = m_params[i].getUsage();
        RC::ConstHandle<Adapter> argAdapter = argTypes[i].getAdapter(), paramAdapter = m_params[i].getAdapter();
        
        if ( argUsage == paramUsage
          && argAdapter->isEquivalentTo( paramAdapter )
          )
          continue;
        
        return false;
      }
      
      return true;
    }
    
    bool Function::isLValueToRValueMatch( ExprTypeVector const &argTypes ) const
    {
      if ( argTypes.size() != m_params.size() )
        return false;
      
      for ( size_t i=0; i<argTypes.size(); ++i )
      {
        Usage argUsage = argTypes[i].getUsage(), paramUsage = m_params[i].getUsage();
        RC::ConstHandle<Adapter> argAdapter = argTypes[i].getAdapter(), paramAdapter = m_params[i].getAdapter();
        
        if ( argUsage == paramUsage
          && argAdapter->isEquivalentTo( paramAdapter )
          )
          continue;
          
        if ( paramUsage == USAGE_RVALUE )
        {
          if ( argUsage == USAGE_LVALUE
            && argAdapter->isEquivalentTo( paramAdapter )
            )
            continue;
        }
          
        return false;
      }
      
      return true;
    }
    
    bool Function::isImplicitCastMatch( ExprTypeVector const &argTypes, ModuleBuilder const &moduleBuilder, size_t &maxCost ) const
    {
      if ( argTypes.size() != m_params.size() )
        return false;
     
      maxCost = 0;
      size_t numExactTypeMatch = 0;
       
      for ( size_t i=0; i<argTypes.size(); ++i )
      {
        Usage argUsage = argTypes[i].getUsage(), paramUsage = m_params[i].getUsage();
        RC::ConstHandle<Adapter> argAdapter = argTypes[i].getAdapter(), paramAdapter = m_params[i].getAdapter();
        
        if ( argUsage == paramUsage
          && argAdapter->isEquivalentTo( paramAdapter )
          )
        {
          ++numExactTypeMatch;
          continue;
        }
          
        if ( paramUsage == USAGE_RVALUE )
        {
          if ( argUsage == USAGE_LVALUE
            && argAdapter->isEquivalentTo( paramAdapter )
            )
          {
            ++numExactTypeMatch;
            continue;
          }

          Function const *function = moduleBuilder.maybeGetPreciseFunction(
            ConstructorPencilKey( paramAdapter ),
            ExprTypeVector(
              ExprType( paramAdapter, USAGE_LVALUE ),
              ExprType( argAdapter, USAGE_RVALUE )
              )
            );
          if ( function )
          {
            maxCost += function->getPolymorphismParameters()->cost;
            continue;
          }
        }
          
        return false;
      }
      
      return numExactTypeMatch >= getPolymorphismParameters()->minExactTypeMatch;
    }
    
    ExprValue Function::llvmCreateCall( BasicBlockBuilder &basicBlockBuilder, std::vector<ExprValue> &args ) const
    {
      RC::Handle<Context> context = basicBlockBuilder.getContext();
      
      if ( m_params.size() != args.size() )
        throw Exception( "incorrect number of arguments (expected %u, actual %u)", (unsigned)m_params.size(), (unsigned)args.size() );

      for ( size_t i=0; i<args.size(); ++i )
      {
        FunctionParam const &param = m_params[i];
        ExprValue &arg = args[i];

        if( !arg.isValid() )
          throw Exception( "expression is invalid" );
        
        try
        {
          arg.castTo( basicBlockBuilder, param.getExprType() );
        }
        catch ( Exception e )
        {
          throw Exception( "argument %u ('%s'): %s", (unsigned)(i + 1), param.getName().c_str(), (const char*)e.getDesc() );
        }
      }

      llvm::Value *returnValue = 0;
      if ( m_returnInfo.usesReturnLValue() )
      {
        RC::ConstHandle<Adapter> returnAdapter = m_returnInfo.getAdapter();
        returnValue = returnAdapter->llvmAlloca( basicBlockBuilder, "resultLValue" );
        returnAdapter->llvmInit( basicBlockBuilder, returnValue );
        args.resize( args.size() + 1, ExprValue(context) );
        for ( size_t i=1; i<args.size(); ++i )
          args[args.size()-i] = args[args.size()-i-1];
        args[0] = ExprValue( returnAdapter, USAGE_LVALUE, context, returnValue );
        basicBlockBuilder.getScope().put( CG::VariableSymbol::Create( args[0] ) );
      }
      
      std::vector<llvm::Value *> argValues;
      for ( size_t i=0; i<args.size(); ++i )
        argValues.push_back( args[i].getValue() );
      
      llvm::Value *resultValue = basicBlockBuilder->CreateCall( m_llvmFunction, argValues.begin(), argValues.end() );
      if( !m_returnInfo.usesReturnLValue() )
        returnValue = resultValue;

      return CG::ExprValue( m_returnInfo.getExprType(), context, returnValue );
    }
  }
}
