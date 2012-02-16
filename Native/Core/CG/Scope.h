/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_SCOPE_H
#define _FABRIC_CG_SCOPE_H

#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/ExprValue.h>
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/Location.h>
#include <Fabric/Core/CG/FunctionParam.h>
#include <Fabric/Core/CG/ReturnInfo.h>
#include <Fabric/Core/CG/Symbol.h>
#include <Fabric/Core/Util/UnorderedMap.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace CG
  {
    class Scope
    {
    public:
    
      Scope( Scope const &parentScope )
        : m_parentScope( &parentScope )
      {
      }

      void put( RC::ConstHandle<Symbol> const &symbol )
      {
        m_symbols.push_back( symbol );
      }
      
      void put( std::string const &name, RC::ConstHandle<Symbol> const &symbol )
      {
        put( symbol );
        m_namedSymbols.insert( StringToSymbolMap::value_type( name, symbol ) );
      }
      
      bool has( std::string const &name ) const
      {
        StringToSymbolMap::const_iterator it = m_namedSymbols.find(name);
        return it != m_namedSymbols.end() || (m_parentScope && m_parentScope->has( name ));
      }
      
      RC::ConstHandle<Symbol> get( std::string const &name ) const
      {
        StringToSymbolMap::const_iterator it = m_namedSymbols.find(name);
        if ( it != m_namedSymbols.end() )
          return it->second;
        else if ( m_parentScope )
          return m_parentScope->get( name );
        else return 0;
      }
      
      void llvmUnwind( BasicBlockBuilder &bbb ) const
      {
        for ( SymbolVector::const_iterator it=m_symbols.begin(); it!=m_symbols.end(); ++it )
        {
          RC::ConstHandle<Symbol> const &symbol = *it;
          if ( symbol->isValue() )
          {
            RC::ConstHandle<ValueSymbol> valueSymbol = RC::ConstHandle<ValueSymbol>::StaticCast( symbol );
            if ( valueSymbol->isVariable() )
            {
              RC::ConstHandle<VariableSymbol> variableSymbol = RC::ConstHandle<VariableSymbol>::StaticCast( valueSymbol );
              ExprValue exprValue = variableSymbol->getExprValue();
              llvm::Value *rValue = 0, *lValue = 0;
              RC::ConstHandle<Adapter> adapter = exprValue.getAdapter();
              switch ( exprValue.getUsage() )
              {
                case USAGE_RVALUE:
                  rValue = exprValue.getValue();
                  lValue = adapter->llvmRValueToLValue( bbb, rValue );
                  break;
                case USAGE_LVALUE:
                  lValue = exprValue.getValue();
                  rValue = adapter->llvmLValueToRValue( bbb, lValue );
                  break;
                default:
                  FABRIC_ASSERT( false );
                  break;
              }
              exprValue.getAdapter()->llvmDispose( bbb, lValue );
            }
          }
        }
      }
            
      virtual void llvmBreak( BasicBlockBuilder &bbb, Location const &location ) const
      {
        llvmUnwind( bbb );
        if ( !m_parentScope )
          throw Error( location, "break outside of loop or switch statment" );
        m_parentScope->llvmBreak( bbb, location );
      }

      virtual void llvmContinue( BasicBlockBuilder &bbb, Location const &location ) const
      {
        llvmUnwind( bbb );
        if ( !m_parentScope )
          throw Error( location, "continue outside of loop" );
        m_parentScope->llvmContinue( bbb, location );
      }

      virtual ReturnInfo const &getReturnInfo() const
      {
        FABRIC_ASSERT( m_parentScope );
        return m_parentScope->getReturnInfo();
      }
            
      virtual llvm::Value *llvmGetReturnLValue() const
      {
        FABRIC_ASSERT( m_parentScope );
        return m_parentScope->llvmGetReturnLValue();
      }
      
      void llvmReturn( BasicBlockBuilder &bbb, ExprValue &exprValue ) const
      {
        ExprType const &returnExprType = getReturnInfo().getExprType();
        if ( returnExprType )
        {
          FABRIC_ASSERT( exprValue );
          RC::ConstHandle<Adapter> returnAdapter = returnExprType.getAdapter();
          llvm::Value *returnRValue = returnAdapter->llvmCast( bbb, exprValue );
          llvm::Value *returnLValue = llvmGetReturnLValue();
          returnAdapter->llvmAssign( bbb, returnLValue, returnRValue );
        }
        else FABRIC_ASSERT( !exprValue );
        llvmReturn( bbb );
      }
      
    protected:
    
      Scope()
        : m_parentScope( 0 )
      {
      }
      
      virtual void llvmReturn( BasicBlockBuilder &bbb ) const
      {
        llvmUnwind( bbb );
        FABRIC_ASSERT( m_parentScope );
        m_parentScope->llvmReturn( bbb );
      }
      
    private:
    
      typedef std::vector< RC::ConstHandle<Symbol> > SymbolVector;
      typedef Util::UnorderedMap< std::string, RC::ConstHandle<Symbol> > StringToSymbolMap;
    
      SymbolVector m_symbols;
      StringToSymbolMap m_namedSymbols;
      Scope const *m_parentScope;
    };
    
    class LoopScope : public Scope
    {
    public:
    
      LoopScope( Scope const &parentScope, llvm::BasicBlock *doneBB, llvm::BasicBlock *nextBB )
        : Scope( parentScope )
        , m_doneBB( doneBB )
        , m_nextBB( nextBB )
      {
      }
      
      virtual void llvmBreak( BasicBlockBuilder &bbb, Location const &location ) const
      {
        bbb->CreateBr( m_doneBB );
      }
      
      virtual void llvmContinue( BasicBlockBuilder &bbb, Location const &location ) const
      {
        bbb->CreateBr( m_nextBB );
      }
      
    private:
    
      llvm::BasicBlock *m_doneBB;
      llvm::BasicBlock *m_nextBB;
    };
    
    class SwitchScope : public Scope
    {
    public:
    
      SwitchScope( Scope const &parentScope, llvm::BasicBlock *breakBB )
        : Scope( parentScope )
        , m_breakBB( breakBB )
      {
      }
      
      virtual void llvmBreak( BasicBlockBuilder &bbb, Location const &location ) const
      {
        bbb->CreateBr( m_breakBB );
      }
      
    private:
    
      llvm::BasicBlock *m_breakBB;
    };
    
    class ModuleScope : public Scope
    {
    public:
    
      ModuleScope()
        : Scope()
      {
      }
    };
    
    class FunctionScope : public Scope
    {
    public:
    
      FunctionScope( ModuleScope const &parentScope, ReturnInfo const &returnInfo )
        : Scope( parentScope )
        , m_returnInfo( returnInfo )
        , m_returnLValue( 0 )
      {
      }
      
      void llvmPrepareReturnLValue( BasicBlockBuilder &bbb )
      {
        RC::ConstHandle<Adapter> returnAdapter = m_returnInfo.getAdapter();
        if ( returnAdapter )
        {
          if ( !m_returnInfo.usesReturnLValue() )
          {
            m_returnLValue = returnAdapter->llvmAlloca( bbb, "returnLValue" );
            returnAdapter->llvmInit( bbb, m_returnLValue );
          }
        }
      }
      
      virtual llvm::Value *llvmGetReturnLValue() const
      {
        RC::ConstHandle<Adapter> returnAdapter = m_returnInfo.getAdapter();
        if ( returnAdapter )
        {
          if ( !m_returnInfo.usesReturnLValue() )
          {
            FABRIC_ASSERT( m_returnLValue );
            return m_returnLValue;
          }
          else return m_returnInfo.getReturnLValue();
        }
        else return 0;
      }
      
      virtual ReturnInfo const &getReturnInfo() const
      {
        return m_returnInfo;
      }
      
    protected:
      
      virtual void llvmReturn( BasicBlockBuilder &bbb ) const
      {
        llvmUnwind( bbb );

        RC::ConstHandle<Adapter> returnAdapter = m_returnInfo.getAdapter();
        if ( returnAdapter )
        {
          if( m_returnInfo.usesReturnLValue() )
          {
            bbb->CreateRetVoid();
          }
          else
          {
            llvm::Value *returnRValue = returnAdapter->llvmLValueToRValue( bbb, llvmGetReturnLValue() );
            bbb->CreateRet( returnRValue );
          }
        }
        else bbb->CreateRetVoid();
      }
      
    private:
    
      ReturnInfo m_returnInfo;
      llvm::Value *m_returnLValue;
    };
  }
}

#endif //_FABRIC_CG_SCOPE_H
