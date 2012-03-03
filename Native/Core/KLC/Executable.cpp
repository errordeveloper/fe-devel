/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/KLC/Executable.h>
#include <Fabric/Core/KLC/ArrayGeneratorOperatorWrapper.h>
#include <Fabric/Core/KLC/ArrayTransformOperatorWrapper.h>
#include <Fabric/Core/KLC/ArrayMapOperatorWrapper.h>
#include <Fabric/Core/KLC/ArrayTransformOperatorWrapper.h>
#include <Fabric/Core/KLC/ReduceOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueGeneratorOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueMapOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueTransformOperatorWrapper.h>
#include <Fabric/Core/DG/IRCache.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/KL/Externals.h>
#include <Fabric/Core/KL/Parser.hpp>
#include <Fabric/Core/KL/StringSource.h>
#include <Fabric/Core/KL/Scanner.h>
#include <Fabric/Core/CG/Diagnostics.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/AST/RequireInfo.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/MT/LogCollector.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/Assembly/Parser.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/PassManager.h>
#include <llvm/Support/StandardPasses.h>

namespace Fabric
{
  namespace KLC
  {
    FABRIC_GC_OBJECT_CLASS_IMPL( Executable, GC::Object )
      
    Util::TLSVar<Executable const *> Executable::s_currentExecutable;
    
    RC::Handle<Executable> Executable::Create(
      GC::Container *gcContainer,
      RC::Handle<CG::Manager> const &cgManager,
      RC::ConstHandle<AST::GlobalList> const &ast,
      CG::CompileOptions const &compileOptions,
      CG::Diagnostics const &diagnostics
      )
    {
      return new Executable(
        FABRIC_GC_OBJECT_MY_CLASS,
        gcContainer,
        cgManager,
        ast,
        compileOptions,
        diagnostics
        );
    }
    
    Executable::Executable(
      FABRIC_GC_OBJECT_CLASS_PARAM,
      GC::Container *gcContainer,
      RC::Handle<CG::Manager> const &cgManager,
      RC::ConstHandle<AST::GlobalList> const &originalAST,
      CG::CompileOptions const &compileOptions,
      CG::Diagnostics const &originalDiagnostics
      )
      : GC::Object( FABRIC_GC_OBJECT_CLASS_ARG )
      , m_gcContainer( gcContainer )
      , m_cgManager( cgManager )
      , m_ast( originalAST )
      , m_diagnostics( originalDiagnostics )
    {
      llvm::InitializeNativeTarget();
      LLVMLinkInJIT();
      
      RC::ConstHandle<RT::Manager> rtManager = cgManager->getRTManager();
      
      AST::RequireNameToLocationMap requires;
      m_ast->collectRequires( requires );

      std::set<std::string> includedRequires;
      while ( !requires.empty() )
      {
        AST::RequireNameToLocationMap::iterator it = requires.begin();
        std::string const &name = it->first;
        if ( includedRequires.find( name ) == includedRequires.end() )
        {
          RC::ConstHandle<AST::GlobalList> requireAST = Plug::Manager::Instance()->maybeGetASTForExt( name );
          if ( !requireAST )
          {
            RC::ConstHandle<RC::Object> typeAST;
            if ( rtManager->maybeGetASTForType( name, typeAST ) )
              requireAST = typeAST? RC::ConstHandle<AST::GlobalList>::StaticCast( typeAST ): AST::GlobalList::Create();
          }
            
          if ( requireAST )
          {
            requireAST->collectRequires( requires );
            m_ast = AST::GlobalList::Create( requireAST, m_ast );
            includedRequires.insert( name );
          }
          else m_diagnostics.addError( it->second, "no registered type or extension named " + _(it->first) );
        }
        requires.erase( it );
      }

      if ( !m_diagnostics.containsError() )
      {
        static const bool optimize = true;
      
        m_cgContext = CG::Context::Create();
        llvm::OwningPtr<llvm::Module> module( new llvm::Module( "Fabric", m_cgContext->getLLVMContext() ) );
        CG::ModuleBuilder moduleBuilder( cgManager, m_cgContext, module.get(), &compileOptions );
#if defined(FABRIC_MODULE_OPENCL)
        OCL::llvmPrepareModule( moduleBuilder, rtManager );
#endif

        llvm::NoFramePointerElim = true;
        llvm::JITExceptionHandling = true;
        
        RC::Handle<DG::IRCache> irCache =  DG::IRCache::Instance( &compileOptions ); 
        std::string irCacheKeyForAST = irCache->keyForAST( m_ast );
        std::string ir = irCache->get( irCacheKeyForAST );
        if ( ir.length() > 0 )
        {
          llvm::SMDiagnostic error;
          llvm::ParseAssemblyString( ir.c_str(), module.get(), error, m_cgContext->getLLVMContext() );
          
          m_ast->registerTypes( cgManager, m_diagnostics );
          FABRIC_ASSERT( !m_diagnostics.containsError() );

          FABRIC_ASSERT( !llvm::verifyModule( *module, llvm::PrintMessageAction ) );
        }
        else
        {
          m_ast->registerTypes( cgManager, m_diagnostics );
          if ( !m_diagnostics.containsError() )
          {
            m_ast->llvmCompileToModule( moduleBuilder, m_diagnostics, false );
          }
          if ( !m_diagnostics.containsError() )
          {
            m_ast->llvmCompileToModule( moduleBuilder, m_diagnostics, true );
          }
          if ( !m_diagnostics.containsError() )
          {
          /*
#if defined(FABRIC_BUILD_DEBUG)
            std::string byteCode;
            llvm::raw_string_ostream byteCodeStream( byteCode );
            module->print( byteCodeStream, 0 );
            byteCodeStream.flush();
#endif
          */
          
            llvm::OwningPtr<llvm::PassManager> passManager( new llvm::PassManager );
            if ( optimize )
            {
              llvm::createStandardAliasAnalysisPasses( passManager.get() );
              llvm::createStandardFunctionPasses( passManager.get(), 3 );
              llvm::createStandardModulePasses( passManager.get(), 3, false, true, true, true, false, llvm::createFunctionInliningPass() );
              llvm::createStandardLTOPasses( passManager.get(), true, true, false );
            }
#if defined(FABRIC_BUILD_DEBUG)
            passManager->add( llvm::createVerifierPass() );
#endif
            passManager->run( *module );
         
            if ( optimize )
            {
              std::string ir;
              llvm::raw_string_ostream irStream( ir );
              module->print( irStream, 0 );
              irStream.flush();
              irCache->put( irCacheKeyForAST, ir );
            }
          }
        }
        
        llvm::Module *llvmModule = module.take();

        std::string errStr;
        m_llvmExecutionEngine.reset(
          llvm::EngineBuilder(llvmModule)
            .setErrorStr(&errStr)
            .setEngineKind(llvm::EngineKind::JIT)
            .create()
          );
        if ( !m_llvmExecutionEngine )
          throw Exception( "Failed to construct ExecutionEngine: " + errStr );

        m_llvmExecutionEngine->InstallLazyFunctionCreator( &LazyFunctionCreator );
        
        // Make sure we don't search loaded libraries for missing symbols. 
        // Only symbols *we* provide should be available as calling functions.
        m_llvmExecutionEngine->DisableSymbolSearching();
        cgManager->llvmAddGlobalMappingsToExecutionEngine( m_llvmExecutionEngine.get(), *llvmModule );
      }
    }

    void Executable::Report( char const *data, size_t length )
    {
      MT::tlsLogCollector.get()->add( data, length );
    }
    
    void *Executable::lazyFunctionCreator( std::string const &functionName ) const
    {
      void *result = 0;
      
      if ( functionName == "report" )
        result = (void *)&Executable::Report;
        
      if ( !result )
        result = KL::LookupExternalSymbol( functionName );
        
      if ( !result )
        result = Plug::Manager::Instance()->llvmResolveExternalFunction( functionName );
        
#if defined(FABRIC_MODULE_OPENCL)
      if ( !result )
        result = OCL::llvmResolveExternalFunction( functionName );
#endif

      if ( !result )
        result = m_cgManager->llvmResolveExternalFunction( functionName );
      
      // We should *always* return a valid symbol. Otherwise something's
      // wrong in the KL compiler/support.
      if( !result )
        throw Exception( "LLVM lookup failed for symbol: " + functionName );

      return result;
    }
    
    void *Executable::LazyFunctionCreator( std::string const &functionName )
    {
      Executable const *currentExecutable = s_currentExecutable;
      FABRIC_ASSERT( currentExecutable );
      return currentExecutable->lazyFunctionCreator( functionName );
    }

    RC::ConstHandle<AST::GlobalList> Executable::getAST() const
    {
      return m_ast;
    }
  
    CG::Diagnostics const &Executable::getDiagnostics() const
    {
      return m_diagnostics;
    }
        
    RC::Handle<CG::Manager> Executable::getCGManager() const
    {
      return m_cgManager;
    }
    
    void Executable::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "getAST", 6 ) )
        jsonExecGetAST( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "getDiagnostics", 14 ) )
        jsonExecGetDiagnostics( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "resolveValueGeneratorOperator", 29 ) )
        jsonExecResolveOperator<ValueGeneratorOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "resolveValueMapOperator", 23 ) )
        jsonExecResolveOperator<ValueMapOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "resolveValueTransformOperator", 29 ) )
        jsonExecResolveOperator<ValueTransformOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "resolveArrayGeneratorOperator", 29 ) )
        jsonExecResolveOperator<ArrayGeneratorOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "resolveArrayMapOperator", 23 ) )
        jsonExecResolveOperator<ArrayMapOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "resolveArrayTransformOperator", 29 ) )
        jsonExecResolveOperator<ArrayTransformOperatorWrapper>( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "resolveReduceOperator", 21 ) )
        jsonExecResolveOperator<ReduceOperatorWrapper>( arg, resultArrayEncoder );
      else GC::Object::jsonExec( cmd, arg, resultArrayEncoder );
    }
    
    void Executable::jsonExecGetDiagnostics(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      JSON::Encoder jg = resultArrayEncoder.makeElement();
      getDiagnostics().encodeJSON( jg );
    }
    
    void Executable::jsonExecGetAST(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      JSON::Encoder jg = resultArrayEncoder.makeElement();
      getAST()->encodeJSON( true, jg );
    }
  }
}
