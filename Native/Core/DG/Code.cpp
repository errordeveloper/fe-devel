/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/DG/Code.h>
#include <Fabric/Core/DG/ExecutionEngine.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/OCL/OCL.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/AST/GlobalList.h>
#include <Fabric/Core/AST/Function.h>
#include <Fabric/Core/AST/Operator.h>
#include <Fabric/Core/AST/RequireInfo.h>
#include <Fabric/Core/RT/Desc.h>
#include <Fabric/Core/RT/Impl.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/Function.h>
#include <Fabric/Core/MT/Impl.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/DG/IRCache.h>
#include <Fabric/Core/KL/StringSource.h>
#include <Fabric/Core/KL/Scanner.h>
#include <Fabric/Core/KL/Parser.hpp>
#include <Fabric/Base/Util/Log.h>

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
  namespace DG
  {
    RC::ConstHandle<Code> Code::Create(
      RC::ConstHandle<Context> const &context,
      std::string const &filename,
      std::string const &sourceCode,
      bool optimizeSynchronously,
      CG::CompileOptions const *compileOptions
      )
    {
      return new Code( context, filename, sourceCode, optimizeSynchronously, compileOptions );
    }

    Code::Code(
      RC::ConstHandle<Context> const &context,
      std::string const &filename,
      std::string const &sourceCode,
      bool optimizeSynchronously,
      CG::CompileOptions const *compileOptions
      )
      : m_contextWeakRef( context )
      , m_filename( filename )
      , m_sourceCode( sourceCode )
      , m_executionEngineAndDiagnosticsMutex( "DG::Code::m_executionEngineAndDiagnosticsMutex" )
      , m_registeredFunctionSetMutex( "DG::Code::m_registeredFunctionSet" )
      , m_optimizeSynchronously( optimizeSynchronously )
      , m_compileOptions( compileOptions )
    {
      llvm::InitializeNativeTarget();
      LLVMLinkInJIT();
      llvm::NoFramePointerElim = true;
      llvm::JITExceptionHandling = true;
      
      FABRIC_ASSERT( m_filename.length() > 0 );
      FABRIC_ASSERT( m_sourceCode.length() > 0 );
      
      RC::ConstHandle<KL::Source> source = KL::StringSource::Create( m_filename, m_sourceCode );
      RC::Handle<KL::Scanner> scanner = KL::Scanner::Create( source );
      m_ast = AST::GlobalList::Create( m_ast, KL::Parse( scanner, m_diagnostics ) );
      if ( !m_diagnostics.containsError() )
        compileAST( m_optimizeSynchronously );
    }
    
    Code::~Code()
    {
    }
    
    void Code::compileAST( bool optimize )
    {
      RC::ConstHandle<Context> context = m_contextWeakRef.makeStrong();
      if ( !context )
        return;
        
      FABRIC_ASSERT( m_ast );
      RC::ConstHandle<AST::GlobalList> ast = m_ast;
      CG::Diagnostics diagnostics;
      
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
            if ( context->getRTManager()->maybeGetASTForType( name, typeAST ) )
              requireAST = typeAST? RC::ConstHandle<AST::GlobalList>::StaticCast( typeAST ): AST::GlobalList::Create();
          }
            
          if ( requireAST )
          {
            requireAST->collectRequires( requires );
            ast = AST::GlobalList::Create( requireAST, ast );
            includedRequires.insert( name );
          }
          else diagnostics.addError( it->second, "no registered type or extension named " + _(it->first) );
        }
        requires.erase( it );
      }

      if ( !diagnostics.containsError() )
      {
        RC::Handle<CG::Manager> cgManager = context->getCGManager();
        RC::Handle<CG::Context> cgContext = CG::Context::Create();
        llvm::OwningPtr<llvm::Module> module( new llvm::Module( "Fabric", cgContext->getLLVMContext() ) );
        CG::ModuleBuilder moduleBuilder( cgManager, cgContext, module.get(), m_compileOptions );
#if defined(FABRIC_MODULE_OPENCL)
        OCL::llvmPrepareModule( moduleBuilder, context->getRTManager() );
#endif

        RC::Handle<IRCache> irCache =  IRCache::Instance( m_compileOptions ); 
        std::string irCacheKeyForAST = irCache->keyForAST( ast );
        std::string ir = irCache->get( irCacheKeyForAST );
        if ( ir.length() > 0 )
        {
          RC::Handle<CG::Manager> cgManager = context->getCGManager();
          
          llvm::SMDiagnostic error;
          llvm::ParseAssemblyString( ir.c_str(), module.get(), error, cgContext->getLLVMContext() );
          
          ast->registerTypes( cgManager, diagnostics );
          FABRIC_ASSERT( !diagnostics.containsError() );

          FABRIC_ASSERT( !llvm::verifyModule( *module, llvm::PrintMessageAction ) );

          linkModule( cgContext, module, true );
          
          return;
        }
        
        ast->registerTypes( cgManager, diagnostics );
        if ( !diagnostics.containsError() )
        {
          ast->llvmCompileToModule( moduleBuilder, diagnostics, false );
        }
        if ( !diagnostics.containsError() )
        {
          ast->llvmCompileToModule( moduleBuilder, diagnostics, true );
        }
        if ( !diagnostics.containsError() )
        {
#if defined(FABRIC_BUILD_DEBUG)
          std::string optimizeByteCode;
          std::string &byteCode = (false && optimize)? optimizeByteCode: m_byteCode;
          llvm::raw_string_ostream byteCodeStream( byteCode );
          module->print( byteCodeStream, 0 );
          byteCodeStream.flush();
#endif
          
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

          linkModule( cgContext, module, optimize );
        }
      }
      
      {
        Util::Mutex::Lock executionEngineAndDiagnosticsMutexLock( m_executionEngineAndDiagnosticsMutex );
        m_diagnostics = diagnostics;
      }
    }
    
    void Code::linkModule( RC::Handle<CG::Context> const &cgContext, llvm::OwningPtr<llvm::Module> &module, bool optimize )
    {
      RC::ConstHandle<Context> context = m_contextWeakRef.makeStrong();
      if ( !context )
        return;
      
      {
        Util::Mutex::Lock executionEngineAndDiagnosticsMutexLock( m_executionEngineAndDiagnosticsMutex );
        RC::ConstHandle<ExecutionEngine> executionEngine = ExecutionEngine::Create( context, cgContext, module.take() );
        
        {
          Util::Mutex::Lock lock( m_registeredFunctionSetMutex );
          for ( RegisteredFunctionSet::const_iterator it=m_registeredFunctionSet.begin();
            it!=m_registeredFunctionSet.end(); ++it )
          {
            Function *function = *it;
            function->onExecutionEngineChange( executionEngine );
          }
        }
        
        m_executionEngine = executionEngine;
      }
      
      if ( !optimize )
      {
        retain();
        MT::ThreadPool::Instance()->executeParallelAsync(
          context->getLogCollector(),
          1,
          &Code::CompileOptimizedAST,
          this,
          MT::ThreadPool::Idle,
          0,
          0
          );
      }
    }
    
    std::string const &Code::getFilename() const
    {
      return m_filename;
    }
    
    std::string const &Code::getSourceCode() const
    {
      return m_sourceCode;
    }
    
#if defined(FABRIC_BUILD_DEBUG)      
    std::string const &Code::getByteCode() const
    {
      return m_byteCode;
    }
#endif
    
    RC::ConstHandle<AST::GlobalList> Code::getAST() const
    {
      return m_ast;
    }
    
    RC::ConstHandle<ExecutionEngine> Code::getExecutionEngine() const
    {
      Util::Mutex::Lock mutexLock( m_executionEngineAndDiagnosticsMutex );
      return m_executionEngine;
    }
    
    CG::Diagnostics const &Code::getDiagnostics() const
    {
      return m_diagnostics;
    }

    void Code::registerFunction( Function *function ) const
    {
      Util::Mutex::Lock lock( m_registeredFunctionSetMutex );
      m_registeredFunctionSet.insert( function );
    }
    
    void Code::unregisterFunction( Function *function ) const
    {
      Util::Mutex::Lock lock( m_registeredFunctionSetMutex );
      RegisteredFunctionSet::iterator it = m_registeredFunctionSet.find( function );
      FABRIC_ASSERT( it != m_registeredFunctionSet.end() );
      m_registeredFunctionSet.erase( it );
    }
  }
}
