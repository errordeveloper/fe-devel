/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_EXPR_VALUE_H
#define _FABRIC_CG_EXPR_VALUE_H

#include <Fabric/Core/CG/ExprType.h>
#include <Fabric/Core/CG/Context.h>

namespace llvm
{
  class Value;
};

namespace Fabric
{
  namespace CG
  {
    class BasicBlockBuilder;
    
    class ExprValue
    {
    public:
    
      ExprValue( RC::Handle<Context> const &context );
      ExprValue( ExprType const &type, RC::Handle<Context> const &context, llvm::Value *value );
      ExprValue( RC::ConstHandle<Adapter> const &adapter, Usage usage, RC::Handle<Context> const &context, llvm::Value *value );
      ExprValue( ExprValue const &that );
      
      ExprValue &operator =( ExprValue const &that );

      void set( BasicBlockBuilder &basicBlockBuilder, RC::ConstHandle<Adapter> const &adapter, Usage usage, llvm::Value *value );
      
      bool isValid() const;
      
      operator bool() const;
      
      bool operator !() const;
      
      ExprType const &getExprType() const;
      
      std::string const &getTypeUserName() const;
      std::string const &getTypeCodeName() const;
      
      RC::ConstHandle<Adapter> getAdapter() const;
      RC::ConstHandle<RT::Desc> getDesc() const;
      RC::ConstHandle<RT::Impl> getImpl() const;
      
      Usage getUsage() const;
      
      llvm::Value *getValue() const;
      
      ExprValue &castTo( BasicBlockBuilder &basicBlockBuilder, ExprType const &exprType );
      ExprValue &castTo( BasicBlockBuilder &basicBlockBuilder, RC::ConstHandle<Adapter> const &adapter );
      ExprValue &castTo( BasicBlockBuilder &basicBlockBuilder, Usage usage );

      void llvmRetain( BasicBlockBuilder &basicBlockBuilder );
      void llvmDispose( BasicBlockBuilder &basicBlockBuilder );
    
      std::string desc() const;
      
    private:
    
      ExprType m_exprType;
      RC::Handle<Context> m_context;
      llvm::Value *m_value;
    };
  };
  
  inline std::string _( CG::ExprValue const &exprValue )
  {
    return exprValue.desc();
  }
};

#endif //_FABRIC_CG_EXPR_VALUE_H
