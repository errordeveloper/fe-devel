/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "FunctionBase.h"
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/CG/PencilSymbol.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Base/Util/SimpleString.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/PassManager.h>

namespace Fabric
{
  namespace AST
  {
    FunctionBase::FunctionBase(
      CG::Location const &location,
      std::string const &returnTypeName,
      std::string const *symbolName,
      RC::ConstHandle<CompoundStatement> const &body,
      bool exportSymbol
      )
      : Global( location )
      , m_returnTypeName( returnTypeName )
      , m_symbolName( symbolName? *symbolName: "" )
      , m_body( body )
      , m_exportSymbol( exportSymbol )
    {
    }
    
    void FunctionBase::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Global::appendJSONMembers( jsonObjectEncoder, includeLocation );
      if ( !m_returnTypeName.empty() )
        jsonObjectEncoder.makeMember( "returnTypeName" ).makeString( m_returnTypeName );
      if ( !m_symbolName.empty() )
        jsonObjectEncoder.makeMember( "symbolName" ).makeString( m_symbolName );
      if ( m_body )
        m_body->appendJSON( jsonObjectEncoder.makeMember( "body" ), includeLocation );
    }
    
    RC::ConstHandle<CompoundStatement> FunctionBase::getBody() const
    {
      return m_body;
    }
    
    std::string const *FunctionBase::getScopeName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return 0;
    }
    
    void FunctionBase::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      if ( !m_returnTypeName.empty() )
      {
        try
        {
          cgManager->getAdapter( m_returnTypeName );
        }
        catch ( Exception e )
        {
          addError( diagnostics, e );
        }
      }

      RC::ConstHandle<AST::ParamVector> params = getParams( cgManager );
      params->registerTypes( cgManager, diagnostics );
      
      if ( m_body )
        m_body->registerTypes( cgManager, diagnostics );
    }
    
    std::string FunctionBase::getSymbolName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      if ( !m_symbolName.empty() )
        return m_symbolName;
      else return getDefaultSymbolName( cgManager );
    }
    
    void FunctionBase::llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const
    {
      RC::Handle<CG::Manager> cgManager = moduleBuilder.getManager();
      
      std::string const *scopeName = getScopeName( cgManager );
      if ( !buildFunctionBodies && scopeName && moduleBuilder.getScope().has( *scopeName ) )
        throw CG::Error( getLocation(), "symbol " + _(*scopeName) + " already exists" );
        
      RC::ConstHandle<CG::Adapter> returnAdapter = getReturnAdapter( cgManager );
      if ( returnAdapter )
        returnAdapter->llvmCompileToModule( moduleBuilder );
      
      std::string pencilKey = getPencilKey( cgManager );
      std::string symbolName = getSymbolName( cgManager );
      std::string desc = getDesc( cgManager );
      RC::ConstHandle<AST::ParamVector> params = getParams( cgManager );
      params->llvmCompileToModule( moduleBuilder, diagnostics, buildFunctionBodies );
      size_t flags = 0;
      if ( m_exportSymbol || !m_body )
        flags |= CG::FunctionBuilder::ExportSymbol;
      CG::FunctionBuilder functionBuilder( moduleBuilder, pencilKey, symbolName, desc, returnAdapter, params->getFunctionParams( moduleBuilder.getManager() ), flags );
      if ( buildFunctionBodies && m_body )
      {
        CG::BasicBlockBuilder basicBlockBuilder( functionBuilder );

        llvm::BasicBlock *basicBlock = functionBuilder.createBasicBlock( "entry" );
        basicBlockBuilder->SetInsertPoint( basicBlock );
        functionBuilder.getScope().llvmPrepareReturnLValue( basicBlockBuilder );
        
        m_body->llvmCompileToBuilder( basicBlockBuilder, diagnostics );

        llvm::BasicBlock *bb = basicBlockBuilder->GetInsertBlock();
        if ( !bb->getTerminator() )
        {
          if ( returnAdapter )
            addError( diagnostics, "not all paths return a value" );
          else
          {
            functionBuilder.getScope().llvmUnwind( basicBlockBuilder );
            basicBlockBuilder->CreateRetVoid();
          }
        }
      }
      
      if ( scopeName )
        moduleBuilder.getScope().put( *scopeName, functionBuilder.getPencil() );
    }

    RC::ConstHandle<CG::Adapter> FunctionBase::getReturnAdapter( RC::Handle<CG::Manager> const &cgManager ) const
    {
      RC::ConstHandle<CG::Adapter> returnAdapter;
      if ( !m_returnTypeName.empty() )
      {
        try
        {
          returnAdapter = cgManager->getAdapter( m_returnTypeName );
        }
        catch ( Exception e )
        {
          throw CG::Error( getLocation(), e );
        }
      }
      return returnAdapter;
    }
  }
}
