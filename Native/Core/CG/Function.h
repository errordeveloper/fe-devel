/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_FUNCTION_H
#define _FABRIC_CG_FUNCTION_H

#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/ExprValue.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/Location.h>
#include <Fabric/Core/CG/FunctionParam.h>
#include <Fabric/Core/CG/ReturnInfo.h>
#include <Fabric/Core/Util/UnorderedMap.h>
#include <Fabric/Base/Exception.h>

#include <vector>

namespace Fabric
{
  namespace CG
  {
    class Function
    {
    public:
      
      Function(
        llvm::Function *llvmFunction,
        ReturnInfo const &returnInfo,
        ParamVector const &params,
        size_t cost
        )
        : m_llvmFunction( llvmFunction )
        , m_returnInfo( returnInfo )
        , m_params( params )
        , m_cost( cost )
      {
      }
      
      Function( Function const &that )
        : m_llvmFunction( that.m_llvmFunction )
        , m_returnInfo( that.m_returnInfo )
        , m_params( that.m_params )
        , m_cost( that.m_cost )
      {
      }
      
      Function &operator =( Function const &that )
      {
        m_llvmFunction = that.m_llvmFunction;
        m_returnInfo = that.m_returnInfo;
        m_params = that.m_params;
        m_cost = that.m_cost;
        return *this;
      }
      
      llvm::Function *getLLVMFunction() const
      {
        return m_llvmFunction;
      }
      
      ReturnInfo const &getReturnInfo() const
      {
        return m_returnInfo;
      }
      
      ParamVector const &getParams() const
      {
        return m_params;
      }
      
      size_t getCost() const
      {
        return m_cost;
      }
      
      void appendParamTypes( ExprTypeVector &paramTypes ) const
      {
        m_params.appendTypes( paramTypes );
      }
      
      ExprTypeVector getParamTypes() const
      {
        ExprTypeVector result;
        appendParamTypes( result );
        return result;
      }
      
      std::string paramsDesc() const
      {
        return getParamTypes().desc();
      }
      
      bool isExactMatch( ExprTypeVector const &argTypes ) const;
      bool isLValueToRValueMatch( ExprTypeVector const &argTypes ) const;
      bool isImplicitCastMatch( ExprTypeVector const &argTypes, ModuleBuilder const &moduleBuilder, size_t &maxCost ) const;
      
      ExprValue llvmCreateCall( BasicBlockBuilder &basicBlockBuilder, std::vector< ExprValue > &args ) const;
      
      ExprValue llvmCreateCall( BasicBlockBuilder &basicBlockBuilder, ExprValue const &arg ) const
      {
        std::vector< ExprValue > args;
        args.push_back( arg );
        return llvmCreateCall( basicBlockBuilder, args );
      }
      
      ExprValue llvmCreateCall( BasicBlockBuilder &basicBlockBuilder, ExprValue const &arg1, ExprValue const &arg2 ) const
      {
        std::vector< ExprValue > args;
        args.push_back( arg1 );
        args.push_back( arg2 );
        return llvmCreateCall( basicBlockBuilder, args );
      }
            
    private:
    
      llvm::Function *m_llvmFunction;
      ReturnInfo m_returnInfo;
      ParamVector m_params;
      size_t m_cost;
    };
    
    typedef std::vector<Function> FunctionVector;
  }
}

#endif //_FABRIC_CG_FUNCTION_H
