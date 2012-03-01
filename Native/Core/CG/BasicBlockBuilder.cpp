/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/PencilSymbol.h>
#include <Fabric/Core/CG/Scope.h>

namespace Fabric
{
  namespace CG
  {
    BasicBlockBuilder::BasicBlockBuilder( FunctionBuilder &functionBuilder )
      : m_functionBuilder( functionBuilder )
      , m_parentBasicBlockBuilder( 0 )
      , m_irBuilder( functionBuilder.getContext()->getLLVMContext() )
      , m_scope( functionBuilder.getScope() )     
    {
    }
    
    BasicBlockBuilder::BasicBlockBuilder( BasicBlockBuilder &parentBasicBlockBuilder, Scope &scope )
      : m_functionBuilder( parentBasicBlockBuilder.getFunctionBuilder() )
      , m_parentBasicBlockBuilder( &parentBasicBlockBuilder )
      , m_irBuilder( parentBasicBlockBuilder.getContext()->getLLVMContext() )
      , m_scope( scope )     
    {
      m_irBuilder.SetInsertPoint( parentBasicBlockBuilder.m_irBuilder.GetInsertBlock() );
    }
    
    BasicBlockBuilder::~BasicBlockBuilder()
    {
      if ( m_parentBasicBlockBuilder )
        m_parentBasicBlockBuilder->m_irBuilder.SetInsertPoint(
          m_irBuilder.GetInsertBlock(),
          m_irBuilder.GetInsertPoint()
          );
    }
    
    llvm::IRBuilder<> *BasicBlockBuilder::operator ->()
    {
      return &m_irBuilder;
    }
    
    Scope const &BasicBlockBuilder::getScope() const
    {
      return m_scope;
    }
    
    Scope &BasicBlockBuilder::getScope()
    {
      return m_scope;
    }
    
    FunctionBuilder &BasicBlockBuilder::getFunctionBuilder()
    {
      return m_functionBuilder;
    }
    
    ModuleBuilder &BasicBlockBuilder::getModuleBuilder()
    {
      return m_functionBuilder.getModuleBuilder();
    }
    
    RC::Handle<Manager> BasicBlockBuilder::getManager()
    {
      return m_functionBuilder.getManager();
    }
    
    RC::ConstHandle<Manager> BasicBlockBuilder::getManager() const
    {
      return m_functionBuilder.getManager();
    }
    
    RC::Handle<Context> BasicBlockBuilder::getContext()
    {
      return m_functionBuilder.getContext();
    }

    RC::ConstHandle<RT::Desc> BasicBlockBuilder::getStrongerTypeOrNone( RC::ConstHandle<RT::Desc> const &lhsDesc, RC::ConstHandle<RT::Desc> const &rhsDesc ) const
    {
      return getManager()->getStrongerTypeOrNone( lhsDesc, rhsDesc );
    }
    
    RC::ConstHandle<Adapter> BasicBlockBuilder::maybeGetAdapter( std::string const &userName ) const
    {
      return m_functionBuilder.maybeGetAdapter( userName );
    }
    
    RC::ConstHandle<Adapter> BasicBlockBuilder::getAdapter( std::string const &userName, CG::Location const &location ) const
    {
      return m_functionBuilder.getAdapter( userName, location );
    }
  };
};
