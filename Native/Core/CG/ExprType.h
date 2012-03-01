/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_EXPR_TYPE_H
#define _FABRIC_CG_EXPR_TYPE_H

#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Base/RC/ConstHandle.h>

#include <string>
#include <vector>

namespace Fabric
{
  namespace RT
  {
    class Desc;
    class Impl;
  };
  
  namespace CG
  {
    enum Usage
    {
      USAGE_UNSPECIFIED,
      USAGE_RVALUE,
      USAGE_LVALUE
    };
    
    inline char const *usageDesc( Usage usage )
    {
      switch ( usage )
      {
        case USAGE_RVALUE:
          return "r-value";
        case USAGE_LVALUE:
          return "l-value";
        case USAGE_UNSPECIFIED:
          return "unspecified";
      }
      return "<unknown usage>";
    }
  
    class ExprType
    {
      RC::ConstHandle<Adapter> m_adapter;
      Usage m_usage;
      
    public:
    
      ExprType();
      ExprType( RC::ConstHandle<Adapter> const &adapter, Usage usage );
      ExprType( ExprType const &that );
      ~ExprType();
      
      ExprType &operator =( ExprType const &that );
      
      void set( RC::ConstHandle<Adapter> const &adapter, Usage usage );

      bool isValid() const;
      operator bool() const;
      bool operator !() const;
      
      RC::ConstHandle<Adapter> getAdapter() const;
      Usage getUsage() const;
      
      RC::ConstHandle<RT::Desc> getDesc() const;
      RC::ConstHandle<RT::Impl> getImpl() const;

      std::string const &getUserName() const;
      std::string const &getCodeName() const;
      
      bool operator ==( ExprType const &that ) const;
      bool operator !=( ExprType const &that ) const;
    };
    
    class ExprTypeVector : public std::vector<ExprType>
    {
    public:
    
      ExprTypeVector()
      {
      }
      
      ExprTypeVector( ExprType const &exprType1 )
      {
        push_back( exprType1 );
      }
      
      ExprTypeVector( ExprType const &exprType1, ExprType const &exprType2 )
      {
        push_back( exprType1 );
        push_back( exprType2 );
      }
      
      ExprTypeVector(
        ExprType const &first,
        ExprTypeVector const &rest
        )
      {
        reserve( 1 + rest.size() );
        push_back( first );
        for ( ExprTypeVector::const_iterator it=rest.begin(); it!=rest.end(); ++it )
          push_back( *it );
      }
      
      AdapterVector getAdapters() const
      {
        AdapterVector result;
        result.reserve( size() );
        for ( const_iterator it=begin(); it!=end(); ++it )
          result.push_back( it->getAdapter() );
        return result;
      }
      
      std::string desc() const;
    };
  }
}

#endif //_FABRIC_CG_EXPR_TYPE_H
