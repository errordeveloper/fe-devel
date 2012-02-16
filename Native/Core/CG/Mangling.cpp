/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/CG/Mangling.h>
#include "Adapter.h"

namespace Fabric
{
  namespace CG
  {
    static inline char const *EncodeUsage( CG::Usage usage )
    {
      switch ( usage )
      {
        case CG::USAGE_LVALUE:
          return "io";
        case CG::USAGE_RVALUE:
          return "in";
        default:
          FABRIC_ASSERT( false );
          return 0;
      }
    }
    
    static std::string EncodeParametersForDefaultSymbolName( ExprTypeVector const &paramTypes )
    {
      std::string result;
      for ( ExprTypeVector::const_iterator it=paramTypes.begin(); it!=paramTypes.end(); ++it )
        result += "__" + std::string( EncodeUsage( it->getUsage() ) ) + "_" + it->getAdapter()->getCodeName();
      return result;
    }
    
    static std::string DescParams( ExprTypeVector const &paramTypes )
    {
      std::string result;
      for ( ExprTypeVector::const_iterator it=paramTypes.begin(); it!=paramTypes.end(); ++it )
      {
        if ( !result.empty() )
          result += ", ";
        if ( it->getUsage() == USAGE_LVALUE )
          result += "io ";
        result += it->getUserName();
      }
      return "(" + result + ")";
    }
    
    static std::string DescParams( AdapterVector const &paramAdapters )
    {
      ExprTypeVector paramTypes;
      for ( AdapterVector::const_iterator it=paramAdapters.begin(); it!=paramAdapters.end(); ++it )
        paramTypes.push_back( ExprType( *it, USAGE_RVALUE ) );
      return DescParams( paramTypes );
    }
    
    // Function

    std::string FunctionPencilKey( std::string const &functionName )
    {
      return "f:" + functionName;
    }

    std::string FunctionDefaultSymbolName(
      std::string const &functionName,
      ExprTypeVector const &paramTypes
      )
    {
      return "__function_" + functionName + EncodeParametersForDefaultSymbolName( paramTypes );
    }

    std::string FunctionFullDesc(
      RC::ConstHandle<Adapter> const &returnAdapter,
      std::string const &functionName,
      ExprTypeVector const &paramTypes
      )
    {
      std::string result = "function ";
      if ( returnAdapter )
        result += returnAdapter->getUserName() + " ";
      result += functionName + DescParams( paramTypes );
      return result;
    }

    std::string FunctionQueryDesc(
      std::string const &functionName,
      ExprTypeVector const &paramTypes
      )
    {
      return "function " + functionName + DescParams( paramTypes );
    }
    
    // Constructor
      
    std::string ConstructorPencilKey( RC::ConstHandle<CG::Adapter> const &thisAdapter )
    {
      return "c:" + thisAdapter->getCodeName();
    }
    
    std::string ConstructorDefaultSymbolName(
      RC::ConstHandle<CG::Adapter> const &thisAdapter,
      AdapterVector const &otherParamAdapters
      )
    {
      CG::ExprTypeVector paramTypes;
      paramTypes.push_back( CG::ExprType( thisAdapter, CG::USAGE_LVALUE ) );
      for ( AdapterVector::const_iterator it = otherParamAdapters.begin(); it != otherParamAdapters.end(); ++it )
        paramTypes.push_back( CG::ExprType( *it, CG::USAGE_RVALUE ) );
      return "__constructor" + EncodeParametersForDefaultSymbolName( paramTypes );
    }
    
    std::string ConstructorFullDesc(
      RC::ConstHandle<CG::Adapter> const &thisAdapter,
      AdapterVector const &paramAdapters
      )
    {
      return "function " + thisAdapter->getUserName() + DescParams( paramAdapters );
    }
    
    std::string ConstructorQueryDesc(
      RC::ConstHandle<CG::Adapter> const &thisAdapter,
      AdapterVector const &paramAdapters
      )
    {
      return "constructor " + thisAdapter->getUserName() + DescParams( paramAdapters );
    }
    
    // Destructor

    std::string DestructorPencilKey( RC::ConstHandle<CG::Adapter> const &thisAdapter )
    {
      return "d:" + thisAdapter->getCodeName();
    }
    
    std::string DestructorDefaultSymbolName( RC::ConstHandle<CG::Adapter> const &thisAdapter )
    {
      CG::ExprTypeVector paramTypes;
      paramTypes.push_back( CG::ExprType( thisAdapter, CG::USAGE_LVALUE ) );
      return "__destructor" + EncodeParametersForDefaultSymbolName( paramTypes );
    }

    std::string DestructorDesc( RC::ConstHandle<CG::Adapter> const &thisAdapter )
    {
      return "function ~" + thisAdapter->getUserName();
    }
    
    // AssOp

    std::string AssignOpPencilKey( RC::ConstHandle<CG::Adapter> const &thisAdapter, AssignOpType type )
    {
      return "a:" + thisAdapter->getCodeName() + ":" + assignOpCodeName(type);
    }
    
    std::string AssignOpDefaultSymbolName(
      RC::ConstHandle<CG::Adapter> const &thisAdapter,
      AssignOpType type,
      RC::ConstHandle<CG::Adapter> const &thatAdapter
      )
    {
      CG::ExprTypeVector paramTypes;
      paramTypes.push_back( CG::ExprType( thisAdapter, CG::USAGE_LVALUE ) );
      paramTypes.push_back( CG::ExprType( thatAdapter, CG::USAGE_RVALUE ) );
      return "__assop_" + assignOpCodeName( type ) + EncodeParametersForDefaultSymbolName( paramTypes );
    }

    std::string AssignOpFullDesc(
      RC::ConstHandle<CG::Adapter> const &thisAdapter,
      AssignOpType type,
      RC::ConstHandle<CG::Adapter> const &thatAdapter
      )
    {
      return "function " + thisAdapter->getUserName() + "." + assignOpUserName( type ) + "(" + thatAdapter->getUserName() + ")";
    }

    std::string AssignOpQueryDesc(
      RC::ConstHandle<CG::Adapter> const &thisAdapter,
      AssignOpType type,
      RC::ConstHandle<CG::Adapter> const &thatAdapter
      )
    {
      return "assignment operator " + thisAdapter->getUserName() + " " + assignOpUserName( type ) + " " + thatAdapter->getUserName();
    }
    
    // UniOp

    std::string UniOpPencilKey( UniOpType type )
    {
      return "u:" + uniOpCodeName(type);
    }
    
    std::string UniOpDefaultSymbolName(
      UniOpType type,
      ExprType const &thisExprType
      )
    {
      CG::ExprTypeVector paramTypes;
      paramTypes.push_back( thisExprType );
      return "__uniop_" + uniOpCodeName(type) + EncodeParametersForDefaultSymbolName( paramTypes );
    }

    std::string UniOpFullDesc(
      RC::ConstHandle<Adapter> const &returnAdapter,
      UniOpType type,
      ExprType const &thisExprType
      )
    {
      std::string result = "function ";
      if ( returnAdapter )
        result += returnAdapter->getUserName() + " ";
      result += uniOpUserName( type ) + DescParams( ExprTypeVector( thisExprType ) );
      return result;
    }

    std::string UniOpQueryDesc(
      UniOpType type,
      ExprType const &thisExprType
      )
    {
      return "unary operator " + uniOpUserName( type ) + DescParams( ExprTypeVector( thisExprType ) );
    }

    // BinOp

    std::string BinOpPencilKey( BinOpType type )
    {
      return "b:" + binOpCodeName(type);
    }
    
    std::string BinOpDefaultSymbolName( BinOpType type, RC::ConstHandle<CG::Adapter> const &lhsAdapter, RC::ConstHandle<CG::Adapter> const &rhsAdapter )
    {
      CG::ExprTypeVector paramTypes;
      paramTypes.push_back( CG::ExprType( lhsAdapter, CG::USAGE_RVALUE ) );
      paramTypes.push_back( CG::ExprType( rhsAdapter, CG::USAGE_RVALUE ) );
      return "__binop_" + binOpCodeName(type) + EncodeParametersForDefaultSymbolName( paramTypes );
    }

    std::string BinOpFullDesc(
      RC::ConstHandle<Adapter> const &returnAdapter,
      BinOpType type,
      RC::ConstHandle<CG::Adapter> const &lhsAdapter,
      RC::ConstHandle<CG::Adapter> const &rhsAdapter
      )
    {
      std::string result = "function ";
      if ( returnAdapter )
        result += returnAdapter->getUserName() + " ";
      result += binOpUserName( type ) + "(" + lhsAdapter->getUserName() + ", " + rhsAdapter->getUserName() + ")";
      return result;
    }

    std::string BinOpQueryDesc(
      BinOpType type,
      RC::ConstHandle<CG::Adapter> const &lhsAdapter,
      RC::ConstHandle<CG::Adapter> const &rhsAdapter
      )
    {
      return "binary operation " + lhsAdapter->getUserName() + " " + binOpUserName( type ) + " " + rhsAdapter->getUserName();
    }
    
    // Method

    std::string MethodPencilKey(
      RC::ConstHandle<CG::Adapter> const &thisAdapter,
      std::string const &name
      )
    {
      return "m:" + thisAdapter->getCodeName() + ":" + name;
    }
    
    std::string MethodDefaultSymbolName(
      CG::ExprType const &thisType,
      std::string const &methodName,
      CG::ExprTypeVector const &otherParamTypes
      )
    {
      CG::ExprTypeVector paramTypes;
      paramTypes.push_back( thisType );
      for ( CG::ExprTypeVector::const_iterator it=otherParamTypes.begin(); it!=otherParamTypes.end(); ++it )
        paramTypes.push_back( *it );
      return "__method_" + methodName + EncodeParametersForDefaultSymbolName( paramTypes );
    }

    std::string MethodFullDesc(
      RC::ConstHandle<Adapter> const &returnAdapter,
      CG::ExprType const &thisType,
      std::string const &methodName,
      CG::ExprTypeVector const &paramTypes
      )
    {
      std::string result = "function ";
      if ( returnAdapter )
        result += returnAdapter->getUserName() + " ";
      result += thisType.getUserName() + "." + methodName + DescParams( paramTypes );
      return result;
    }

    std::string MethodQueryDesc(
      CG::ExprType const &thisType,
      std::string const &methodName,
      CG::ExprTypeVector const &paramTypes
      )
    {
      return "method " + thisType.getUserName() + "." + methodName + DescParams( paramTypes );
    }
  }
}
