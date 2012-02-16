/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/Core/DG/ExecutionEngine.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/KL/Externals.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/MT/ParallelCall.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/OCL/OCL.h>

#include <llvm/ExecutionEngine/JIT.h>

// [pzion 20110307] Include this last because it does lots of
// #defines on Linux that mess up llvm
//#include <Fabric/Core/OGL/OGL.h>

#if defined( FABRIC_OS_WINDOWS )
extern "C" void _chkstk( );
#endif

namespace Fabric
{
  namespace DG
  {
    Util::Mutex ExecutionEngine::s_llvmJITMutex( "DG::ExecutionEngine::s_llvmJITMutex" );
    Util::TLSVar< RC::ConstHandle<Context> > ExecutionEngine::s_currentContext;
    
    void ExecutionEngine::Report( char const *data, size_t length )
    {
      Context *context = static_cast<Context *>( MT::ParallelCall::GetUserdata() );
      context->getLogCollector()->add( data, length );
    }
    
    void *ExecutionEngine::LazyFunctionCreator( std::string const &functionName )
    {
      if ( functionName == "report" )
        return (void *)&ExecutionEngine::Report;
      else
      {
        void *result = 0;
        if ( !result )
          result = KL::LookupExternalSymbol( functionName );
        if ( !result )
          result = Plug::Manager::Instance()->llvmResolveExternalFunction( functionName );
#if defined(FABRIC_MODULE_OPENCL)
        if ( !result )
          result = OCL::llvmResolveExternalFunction( functionName );
#endif
        if ( !result )
        {
          RC::ConstHandle<Context> context = s_currentContext;
          result = context->getCGManager()->llvmResolveExternalFunction( functionName );
        }
        
        // We should *always* return a valid symbol. Otherwise something's
        // wrong in the KL compiler/support.
        if( !result )
          throw Exception( "LLVM lookup failed for symbol: " + functionName );

        return result;
      }
    }

    RC::ConstHandle<ExecutionEngine> ExecutionEngine::Create( RC::ConstHandle<Context> const &context, RC::Handle<CG::Context> const &cgContext, llvm::Module *llvmModule )
    {
      return new ExecutionEngine( context, cgContext, llvmModule );
    }
    
    ExecutionEngine::ExecutionEngine( RC::ConstHandle<Context> const &context, RC::Handle<CG::Context> const &cgContext, llvm::Module *llvmModule )
      : m_contextWeakRef( context )
      , m_cgContext( cgContext )
    {
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
      context->getCGManager()->llvmAddGlobalMappingsToExecutionEngine( m_llvmExecutionEngine.get(), *llvmModule );
    }
    
    ExecutionEngine::GenericFunctionPtr ExecutionEngine::getFunctionByName( std::string const &functionName ) const
    {
      RC::ConstHandle<Context> context = m_contextWeakRef.makeStrong();
      if ( !context )
        return 0;
      
      {
        Util::Mutex::Lock llvmJITMutexLock( s_llvmJITMutex );
          
        ContextSetter contextSetter( context );
        
        GenericFunctionPtr result = 0;
        llvm::Function *llvmFunction = m_llvmExecutionEngine->FindFunctionNamed( functionName.c_str() );
        if ( llvmFunction )
          result = GenericFunctionPtr( m_llvmExecutionEngine->getPointerToFunction( llvmFunction ) );
        return result;
      }
    }

    RC::ConstHandle<Context> ExecutionEngine::GetCurrentContext()
    {
      return s_currentContext;
    }

    ExecutionEngine::ContextSetter::ContextSetter( RC::ConstHandle<Context> const &context )
    {
      m_oldContext = s_currentContext;
      s_currentContext = context;
    }
    
    ExecutionEngine::ContextSetter::~ContextSetter()
    {
      s_currentContext = m_oldContext;
    }
  };
};
