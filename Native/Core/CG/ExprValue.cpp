/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "ExprValue.h"
#include "Adapter.h"

#include <Fabric/Base/Util/Format.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Core/RT/Impl.h>
#include <Fabric/Base/Exception.h>

#include <llvm/Value.h>
#include <llvm/Type.h>

namespace Fabric
{
  namespace CG
  {
    ExprValue::ExprValue( RC::Handle<Context> const &context )
      : m_context( context )
      , m_value( 0 )
    {
    }
  
    ExprValue::ExprValue( ExprType const &exprType, RC::Handle<Context> const &context, llvm::Value *value )
      : m_exprType( exprType )
      , m_context( context )
      , m_value( value )
    {
    /*
      if (!( !m_exprType
        || m_exprType.getUsage() == USAGE_RVALUE && m_value->getType() == m_exprType.getAdapter()->llvmRType( m_context )
        || m_exprType.getUsage() == USAGE_LVALUE && m_value->getType() == m_exprType.getAdapter()->llvmLType( m_context )
        ))
      {
        m_value->getType()->dump(); 
        m_exprType.getAdapter()->llvmRType( m_context )->dump(); 
        m_exprType.getAdapter()->llvmLType( m_context )->dump(); 
      }
    */
      FABRIC_ASSERT( !m_exprType
        || m_exprType.getUsage() == USAGE_RVALUE && m_value->getType() == m_exprType.getAdapter()->llvmRType( m_context )
        || m_exprType.getUsage() == USAGE_LVALUE && m_value->getType() == m_exprType.getAdapter()->llvmLType( m_context )
        );
    }
    
    ExprValue::ExprValue( RC::ConstHandle<Adapter> const &adapter, Usage usage, RC::Handle<Context> const &context, llvm::Value *value )
      : m_exprType( adapter, usage )
      , m_context( context )
      , m_value( value )
    {
      FABRIC_ASSERT( !m_exprType
        || m_exprType.getUsage() == USAGE_RVALUE && m_value->getType() == m_exprType.getAdapter()->llvmRType( m_context )
        || m_exprType.getUsage() == USAGE_LVALUE && m_value->getType() == m_exprType.getAdapter()->llvmLType( m_context )
        );
    }
    
    ExprValue::ExprValue( ExprValue const &that )
      : m_exprType( that.m_exprType )
      , m_context( that.m_context )
      , m_value( that.m_value )
    {
    }
    
    ExprValue &ExprValue::operator =( ExprValue const &that )
    {
      m_exprType = that.m_exprType;
      m_value = that.m_value;
      return *this;
    }
    
    bool ExprValue::isValid() const
    {
      return m_exprType.isValid() && m_value;
    }
    
    ExprValue::operator bool() const
    {
      return isValid();
    }
    
    bool ExprValue::operator !() const
    {
      return !isValid();
    }
    
    ExprType const &ExprValue::getExprType() const
    {
      return m_exprType;
    }
    
    std::string const &ExprValue::getTypeUserName() const
    {
      return getExprType().getUserName();
    }
      
    std::string const &ExprValue::getTypeCodeName() const
    {
      return getExprType().getCodeName();
    }
      
    Usage ExprValue::getUsage() const
    {
      return getExprType().getUsage();
    }
    
    llvm::Value *ExprValue::getValue() const
    {
      return m_value;
    }

    void ExprValue::llvmDispose( BasicBlockBuilder &basicBlockBuilder )
    {
      if ( isValid() )
      {
        getAdapter()->llvmDispose( basicBlockBuilder, getValue() );
        m_value = 0;
      }
    }
    
    RC::ConstHandle<Adapter> ExprValue::getAdapter() const
    {
      return m_exprType.getAdapter();
    }
    
    RC::ConstHandle<RT::Desc> ExprValue::getDesc() const
    {
      return getAdapter()->getDesc();
    }
    
    RC::ConstHandle<RT::Impl> ExprValue::getImpl() const
    {
      return getDesc()->getImpl();
    }

    ExprValue &ExprValue::castTo( BasicBlockBuilder &basicBlockBuilder, ExprType const &dstExprType )
    {
      RC::ConstHandle<Adapter> srcAdapter = getAdapter();
      Usage srcUsage = getUsage();
      
      RC::ConstHandle<Adapter> dstAdapter = dstExprType.getAdapter();
      if ( !dstAdapter )
        dstAdapter = srcAdapter;
      Usage dstUsage = dstExprType.getUsage();
      if ( dstUsage == USAGE_UNSPECIFIED )
        dstUsage = srcUsage;
      
      if ( m_value )
      {
        if ( !dstAdapter )
        {
          //
        }
        else if ( srcAdapter->getImpl() != dstAdapter->getImpl() )
        {
          switch ( dstUsage )
          {
            case USAGE_RVALUE:
            {
              m_value = dstAdapter->llvmCast( basicBlockBuilder, *this );
            }
            break;
            
            case USAGE_LVALUE:
              throw Exception( "incorrect type (expected "+dstAdapter->getUserName()+", actual "+srcAdapter->getUserName()+")" );
              break;
            
            case USAGE_UNSPECIFIED:
              FABRIC_ASSERT( false );
              throw Exception( "unspecified usage" );
          }
        }
        else if ( dstUsage != USAGE_UNSPECIFIED && dstUsage != srcUsage )
        {
          switch ( dstUsage )
          {
            case USAGE_RVALUE:
            {
              llvm::Value *rValue = srcAdapter->llvmLValueToRValue( basicBlockBuilder, m_value );
              m_value = rValue;
            }
            break;
            
            case USAGE_LVALUE:
              throw Exception( "expression is not an l-value" );
              break;
            
            case USAGE_UNSPECIFIED:
              FABRIC_ASSERT( false );
              throw Exception( "unspecified usage" );
          }
        }
      }
      
      m_exprType.set( dstAdapter, dstUsage );
      
      return *this;
    }
    
    ExprValue &ExprValue::castTo( BasicBlockBuilder &basicBlockBuilder, RC::ConstHandle<Adapter> const &adapter )
    {
      ExprType exprType( adapter, getUsage() );
      return castTo( basicBlockBuilder, exprType );
    }
    
    ExprValue &ExprValue::castTo( BasicBlockBuilder &basicBlockBuilder, Usage usage )
    {
      ExprType exprType( getAdapter(), usage );
      return castTo( basicBlockBuilder, exprType );
    }
  };
};
