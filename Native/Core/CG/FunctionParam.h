/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_FUNCTION_PARAM_H
#define _FABRIC_CG_FUNCTION_PARAM_H

#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/ExprType.h>
#include <Fabric/Base/Exception.h>

#include <string>

namespace llvm
{
  class Type;
}

namespace Fabric
{
  namespace CG
  {
    class Context;
    
    class FunctionParam
    {
    public:
    
      FunctionParam( std::string const &name, ExprType const &exprType )
        : m_name( name )
        , m_exprType( exprType )
      {
      }
    
      FunctionParam( std::string const &name, RC::ConstHandle<Adapter> const &adapter, Usage usage )
        : m_name( name )
        , m_exprType( adapter, usage )
      {
      }
      
      FunctionParam( FunctionParam const &that )
        : m_name( that.m_name )
        , m_exprType( that.m_exprType )
      {
      }
      
      FunctionParam &operator =( FunctionParam const &that )
      {
        m_name = that.m_name;
        m_exprType = that.m_exprType;
        return *this;
      }
    
      std::string const &getName() const
      {
        return m_name;
      }
      
      ExprType const &getExprType() const
      {
        return m_exprType;
      }
      
      std::string const &getTypeUserName() const
      {
        return getExprType().getUserName();
      }
      
      std::string const &getTypeCodeName() const
      {
        return getExprType().getCodeName();
      }
      
      RC::ConstHandle<Adapter> getAdapter() const
      {
        return getExprType().getAdapter();
      }
      
      Usage getUsage() const
      {
        return getExprType().getUsage();
      }
      
      llvm::Type const *getLLVMType( RC::Handle<Context> const &context ) const
      {
        switch ( getUsage() )
        {
          case USAGE_RVALUE:
            return getAdapter()->llvmRType( context );
          case USAGE_LVALUE:
            return getAdapter()->llvmLType( context );
          case USAGE_UNSPECIFIED:
            FABRIC_ASSERT( false );
            throw Exception( "usage unspecified" );
        }
        return 0;
      }

    private:
    
      std::string m_name;
      ExprType m_exprType;
    };
    
    class ParamVector : public std::vector<FunctionParam>
    {
    public:
    
      ParamVector()
      {
      }
      
      ParamVector( FunctionParam const &param )
      {
        push_back( param );
      }
      
      ParamVector(
        FunctionParam const &param1,
        FunctionParam const &param2
        )
      {
        push_back( param1 );
        push_back( param2 );
      }
      
      ParamVector(
        FunctionParam const &param1,
        FunctionParam const &param2,
        FunctionParam const &param3
        )
      {
        push_back( param1 );
        push_back( param2 );
        push_back( param3 );
      }
      
      ParamVector(
        FunctionParam const &param1,
        FunctionParam const &param2,
        FunctionParam const &param3,
        FunctionParam const &param4
        )
      {
        push_back( param1 );
        push_back( param2 );
        push_back( param3 );
        push_back( param4 );
      }
      
      ParamVector( ParamVector const &that )
      {
        reserve( that.size() );
        for ( const_iterator it=that.begin(); it!=that.end(); ++it )
          push_back( *it );
      }
      
      ParamVector &operator =( ParamVector const &that )
      {
        reserve( that.size() );
        for ( const_iterator it=that.begin(); it!=that.end(); ++it )
          push_back( *it );
        return *this;
      }
      
      void appendAdapters( std::vector< RC::ConstHandle<CG::Adapter> > &adapters ) const
      {
        for ( const_iterator it=begin(); it!=end(); ++it )
          adapters.push_back( it->getAdapter() );
      }
      
      std::vector< RC::ConstHandle<CG::Adapter> > getAdapters() const
      {
        std::vector< RC::ConstHandle<CG::Adapter> > result;
        appendAdapters( result );
        return result;
      }
      
      void appendTypes( ExprTypeVector &paramTypes ) const
      {
        for ( const_iterator it=begin(); it!=end(); ++it )
          paramTypes.push_back( it->getExprType() );
      }
      
      ExprTypeVector getTypes() const
      {
        ExprTypeVector result;
        appendTypes( result );
        return result;
      }
    };
  }
}

#endif //_FABRIC_CG_FUNCTION_PARAM_H
