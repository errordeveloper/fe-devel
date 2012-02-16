#include "IndexOp.h"
#include <Fabric/Core/CG/ArrayAdapter.h>
#include <Fabric/Core/CG/DictAdapter.h>
#include <Fabric/Core/CG/SizeAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/RT/ImplType.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( IndexOp );
    
    RC::ConstHandle<IndexOp> IndexOp::Create(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &expr,
      RC::ConstHandle<Expr> const &indexExpr
      )
    {
      return new IndexOp( location, expr, indexExpr );
    }
    
    IndexOp::IndexOp( CG::Location const &location, RC::ConstHandle<Expr> const &expr, RC::ConstHandle<Expr> const &indexExpr )
      : Expr( location )
      , m_expr( expr )
      , m_indexExpr( indexExpr )
    {
    }
    
    void IndexOp::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Expr::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_expr->appendJSON( jsonObjectEncoder.makeMember( "expr" ), includeLocation );
      m_indexExpr->appendJSON( jsonObjectEncoder.makeMember( "indexExpr" ), includeLocation );
    }
    
    void IndexOp::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_expr->registerTypes( cgManager, diagnostics );
      m_indexExpr->registerTypes( cgManager, diagnostics );
    }
    
    CG::ExprType IndexOp::getExprType( CG::BasicBlockBuilder &basicBlockBuilder ) const
    {
      CG::ExprType exprType = m_expr->getExprType( basicBlockBuilder );
      exprType.getAdapter()->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      
      if ( RT::isArray( exprType.getAdapter()->getType() ) )
      {
        RC::ConstHandle<CG::ArrayAdapter> arrayType = RC::ConstHandle<CG::ArrayAdapter>::StaticCast( exprType.getAdapter() );
        
        RC::ConstHandle<CG::Adapter> memberAdapter = arrayType->getMemberAdapter();
        memberAdapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
        return CG::ExprType( memberAdapter, exprType.getUsage() );
      }
      else if ( RT::isDict( exprType.getAdapter()->getType() ) )
      {
        RC::ConstHandle<CG::DictAdapter> dictType = RC::ConstHandle<CG::DictAdapter>::StaticCast( exprType.getAdapter() );
        
        RC::ConstHandle<CG::Adapter> valueAdapter = dictType->getValueAdapter();
        valueAdapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
        return CG::ExprType( valueAdapter, exprType.getUsage() );
      }
      else throw Exception( "only arrays and dictionaries can be indexed" );
    }
    
    CG::ExprValue IndexOp::buildExprValue( CG::BasicBlockBuilder &basicBlockBuilder, CG::Usage usage, std::string const &lValueErrorDesc ) const
    {
      CG::ExprValue result( basicBlockBuilder.getContext() );
      try
      {
        CG::ExprValue arrayExprValue = m_expr->buildExprValue( basicBlockBuilder, usage, lValueErrorDesc );
        RC::ConstHandle<CG::Adapter> adapter = arrayExprValue.getAdapter();
        
        if ( RT::isArray( arrayExprValue.getAdapter()->getType() ) )
        {
          RC::ConstHandle<CG::ArrayAdapter> arrayAdapter = RC::ConstHandle<CG::ArrayAdapter>::StaticCast( adapter );
          
          CG::ExprValue indexExprValue = m_indexExpr->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );
          RC::ConstHandle< CG::SizeAdapter > sizeAdapter = basicBlockBuilder.getManager()->getSizeAdapter();
          llvm::Value *indexExprRValue = sizeAdapter->llvmCast( basicBlockBuilder, indexExprValue );
          if ( indexExprRValue )
          {
            switch ( usage )
            {
              case CG::USAGE_LVALUE:
                result = CG::ExprValue(
                  arrayAdapter->getMemberAdapter(),
                  CG::USAGE_LVALUE,
                  basicBlockBuilder.getContext(),
                  arrayAdapter->llvmNonConstIndexOp(
                    basicBlockBuilder,
                    arrayExprValue.getValue(),
                    indexExprRValue,
                    &getLocation()
                    )
                  );
                break;
              default:
                result = CG::ExprValue(
                  arrayAdapter->getMemberAdapter(),
                  CG::USAGE_RVALUE,
                  basicBlockBuilder.getContext(),
                  arrayAdapter->llvmConstIndexOp(
                    basicBlockBuilder,
                    arrayExprValue.getValue(),
                    indexExprRValue,
                    &getLocation()
                    )
                  );
                break;
            }
          }
        }
        else if ( RT::isDict( arrayExprValue.getAdapter()->getType() ) )
        {
          RC::ConstHandle<CG::DictAdapter> dictAdapter = RC::ConstHandle<CG::DictAdapter>::StaticCast( adapter );
          
          CG::ExprValue indexExprValue = m_indexExpr->buildExprValue( basicBlockBuilder, CG::USAGE_RVALUE, lValueErrorDesc );
          RC::ConstHandle<CG::ComparableAdapter> keyAdapter = dictAdapter->getKeyAdapter();
          llvm::Value *indexExprRValue = keyAdapter->llvmCast( basicBlockBuilder, indexExprValue );
          if ( indexExprRValue )
          {
            switch ( usage )
            {
              case CG::USAGE_LVALUE:
                result = CG::ExprValue(
                  dictAdapter->getValueAdapter(),
                  CG::USAGE_LVALUE,
                  basicBlockBuilder.getContext(),
                  dictAdapter->llvmGetLValue(
                    basicBlockBuilder,
                    arrayExprValue.getValue(),
                    indexExprRValue
                    )
                  );
                break;
              default:
                result = CG::ExprValue(
                  dictAdapter->getValueAdapter(),
                  CG::USAGE_RVALUE,
                  basicBlockBuilder.getContext(),
                  dictAdapter->llvmGetRValue(
                    basicBlockBuilder,
                    arrayExprValue.getValue(),
                    indexExprRValue
                    )
                  );
                break;
            }
          }
        }
        else throw CG::Error( getLocation(), "only arrays and dictionaries can be indexed" );
      }
      catch ( CG::Error e )
      {
        throw e;
      }
      catch ( Exception e )
      {
        throw CG::Error( getLocation(), e );
      }
      return result;
    }
  };
};
