/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <stdio.h>
#include <Fabric/Core/KL/Compiler.h>
#include <Fabric/Core/KL/Externals.h>
#include <Fabric/Core/KL/Parser.hpp>
#include <Fabric/Core/KL/Scanner.h>
#include <Fabric/Core/KL/StringSource.h>
#include <Fabric/Core/AST/GlobalList.h>
#include <Fabric/Core/AST/RequireInfo.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/RT/NumericDesc.h>
#include <Fabric/Core/RT/StringDesc.h>
#include <Fabric/Core/RT/StructDesc.h>
#include <Fabric/Core/RT/OpaqueDesc.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/CG/CompileOptions.h>
#include <Fabric/Core/CG/Diagnostics.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/OCL/OCL.h>
#include <Fabric/Core/OCL/Debug.h>

#include <memory>

#undef DestroyAll
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Target/TargetRegistry.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Target/TargetSelect.h>
#include <llvm/Target/TargetData.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/PassManager.h>
#include <llvm/Support/StandardPasses.h>
#include <llvm/Support/raw_ostream.h>
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

#include <getopt.h>

using namespace Fabric;
using namespace Fabric::KL;

enum RunFlags
{
  RF_Run         = 1 << 0,
  RF_Verbose     = 1 << 1,
  RF_ShowASM     = 1 << 2,
  RF_ShowAST     = 1 << 3,
  RF_ShowBison   = 1 << 4,
  RF_ShowIR      = 1 << 5,
  RF_ShowOptIR   = 1 << 6,
  RF_ShowOptASM	 = 1 << 7,
  RF_ShowTokens	 = 1 << 8
};

static CG::CompileOptions compileOptions;


void dumpDiagnostics( CG::Diagnostics const &diagnostics )
{
  for ( CG::Diagnostics::const_iterator it=diagnostics.begin(); it!=diagnostics.end(); ++it )
  {
    CG::Location const &location = it->first;
    CG::Diagnostic const &diagnostic = it->second;
    printf( "%s:%u:%u: %s: %s\n", location.getFilename()->c_str(), (unsigned)location.getLine(), (unsigned)location.getColumn(), diagnostic.getLevelDesc(), (const char*)diagnostic.getDesc() );
  }
}

RC::Handle<CG::Manager> cgManager;

static float externalSquare( float value )
{
  return value * value;
}

static void report( char const *data, size_t length )
{
  fwrite( data, 1, length, stdout );
  fputc( '\n', stdout );
  fflush( stdout );
}

static void *LazyFunctionCreator( std::string const &functionName )
{
  if ( functionName == "report" )
    return (void *)&report;
  else if ( functionName == "externalSquare" )
    return (void *)&externalSquare;
  else
  {
    void *result = KL::LookupExternalSymbol( functionName );
    if ( result )
      return result;
    result = cgManager->llvmResolveExternalFunction( functionName );
    if ( result )
      return result;
#if defined(FABRIC_MODULE_OPENCL)
    result = OCL::llvmResolveExternalFunction( functionName );
    if ( result )
      return result;
#endif
  }
  throw Exception( "Unable to look up symbol for '%s'", functionName.c_str() );
  return 0;
}


void handleFile( std::string const &filename, FILE *fp, unsigned int runFlags )
{
  std::string sourceString;
  while ( true )
  {
    char buffer[16384];
    size_t count = fread( buffer, 1, 16384, fp );
    sourceString.append( &buffer[0], count );
    if ( count < 16384 )
      break;
  }
  
  RC::ConstHandle<KL::Source> source = KL::StringSource::Create( filename, sourceString );
  if ( runFlags & RF_ShowTokens )
  {
    RC::Handle<KL::Scanner> scanner = KL::Scanner::Create( source );
    for (;;)
    {
      KL::Token token = scanner->nextToken();
      std::string escValue = _(token.getSourceRange().toString());
      printf( "%u: %s\n", unsigned(token.getType()), escValue.c_str() );
      if ( token.getType() == TOKEN_END )
        break;
    }
  }

  RC::Handle<KL::Scanner> scanner = KL::Scanner::Create( source );
  CG::Diagnostics diagnostics;
  
  llvm::CodeGenOpt::Level    optLevel = llvm::CodeGenOpt::Aggressive;

  
  llvm::InitializeNativeTarget();
  llvm::InitializeAllAsmPrinters();
  llvm::NoFramePointerElim = true;
  llvm::JITExceptionHandling = true;

  RC::Handle<RT::Manager> rtManager = RT::Manager::Create( KL::Compiler::Create() );
  cgManager = CG::Manager::Create( rtManager );
  RC::Handle<CG::Context> cgContext = CG::Context::Create();
  std::auto_ptr<llvm::Module> module( new llvm::Module( "kl", cgContext->getLLVMContext() ) );

  CG::ModuleBuilder moduleBuilder( cgManager, cgContext, module.get(), &compileOptions );
  
#if defined(FABRIC_MODULE_OPENCL)
  OCL::llvmPrepareModule( moduleBuilder, rtManager );
#endif
  
  RC::ConstHandle<AST::GlobalList> globalList = KL::Parse( scanner, diagnostics );
  if ( diagnostics.containsError() )
  {
    dumpDiagnostics( diagnostics );
    return;
  }
  
  if ( runFlags & RF_ShowAST )
  {
    if ( runFlags & RF_Verbose )
      printf( "-- AST --\n" );
    Util::SimpleString globalListJSONString = globalList->toJSON( true );
    printf( "%s\n", globalListJSONString.getCString() );
  }

  if( runFlags & (RF_ShowASM | RF_ShowIR | RF_ShowOptIR | RF_ShowOptASM | RF_Run) )
  {
    AST::RequireNameToLocationMap requires;
    globalList->collectRequires( requires );
    for ( AST::RequireNameToLocationMap::const_iterator it=requires.begin(); it!=requires.end(); ++it )
      diagnostics.addError( it->second, "no registered type or extension named " + _(it->first) );
    
    if ( !diagnostics.containsError() )
      globalList->registerTypes( cgManager, diagnostics );
    if ( !diagnostics.containsError() )
      globalList->llvmCompileToModule( moduleBuilder, diagnostics, false );
    if ( !diagnostics.containsError() )
      globalList->llvmCompileToModule( moduleBuilder, diagnostics, true );
    dumpDiagnostics( diagnostics );
    if ( !diagnostics.containsError() )
    {   
      if ( runFlags & RF_ShowIR )
      {
        if ( runFlags & RF_Verbose )
          printf( "-- Unoptimized IR --\n" ); 
        std::string irString;
        llvm::raw_string_ostream byteCodeStream( irString );
        module->print( byteCodeStream, 0 );
        byteCodeStream.flush();
        printf( "%s", irString.c_str() );
      }
      
      if( runFlags & RF_ShowASM )
      {
        std::string   errorStr;
        std::string   targetTriple = llvm::sys::getHostTriple();
        const llvm::Target *target = llvm::TargetRegistry::lookupTarget( targetTriple, errorStr );
        if( !target )
          throw Exception( errorStr );

        llvm::TargetMachine *targetMachine = target->createTargetMachine( targetTriple, "" );
        targetMachine->setAsmVerbosityDefault( true );

        if ( runFlags & RF_Verbose )
          printf( "-- Unoptimized %s assembly --\n", llvm::sys::getHostTriple().c_str() ); 

        llvm::FunctionPassManager *functionPassManager = new llvm::FunctionPassManager( module.get() );
        targetMachine->addPassesToEmitFile( 
          *functionPassManager, llvm::fouts(), 
          llvm::TargetMachine::CGFT_AssemblyFile, llvm::CodeGenOpt::Default );
        functionPassManager->doInitialization();
      
        llvm::Module::FunctionListType &functionList = module->getFunctionList();
        for ( llvm::Module::FunctionListType::iterator it=functionList.begin(); it!=functionList.end(); ++it )
          functionPassManager->run( *it );

        delete functionPassManager;
      }

      std::string errStr;
      llvm::EngineBuilder builder( module.get() );
      
      builder.setErrorStr( &errStr );
      builder.setEngineKind( llvm::EngineKind::JIT );
      builder.setOptLevel( optLevel );

      llvm::ExecutionEngine *executionEngine = builder.create();
      if ( !executionEngine )
        throw Fabric::Exception( "Failed to construct ExecutionEngine: " + errStr );
      
      executionEngine->InstallLazyFunctionCreator( &LazyFunctionCreator );
      cgManager->llvmAddGlobalMappingsToExecutionEngine( executionEngine, *module );
      
      // Make sure we don't search loaded libraries for missing symbols. 
      // Only symbols *we* provide should be available as calling functions.
      executionEngine->DisableSymbolSearching( );

      llvm::OwningPtr<llvm::PassManager> passManager( new llvm::PassManager );
      passManager->add( llvm::createVerifierPass() );
      llvm::createStandardAliasAnalysisPasses( passManager.get() );
      llvm::createStandardFunctionPasses( passManager.get(), 3 );
      llvm::createStandardModulePasses( passManager.get(), 3, false, true, true, true, false, llvm::createFunctionInliningPass() );
      llvm::createStandardLTOPasses( passManager.get(), true, true, false );
      passManager->run( *module );

      if( runFlags & RF_ShowOptASM )
      {
        std::string   errorStr;
        std::string   targetTriple = llvm::sys::getHostTriple();
        const llvm::Target *target = llvm::TargetRegistry::lookupTarget( targetTriple, errorStr );
        if( !target )
          throw Exception( errorStr );

        llvm::TargetMachine *targetMachine = target->createTargetMachine( targetTriple, "" );
        targetMachine->setAsmVerbosityDefault( true );

        if ( runFlags & RF_Verbose )
          printf( "-- Optimized %s assembly --\n", llvm::sys::getHostTriple().c_str() ); 

        llvm::FunctionPassManager *functionPassManager = new llvm::FunctionPassManager( module.get() );
        targetMachine->addPassesToEmitFile( 
          *functionPassManager, llvm::fouts(), 
          llvm::TargetMachine::CGFT_AssemblyFile, optLevel );
        functionPassManager->doInitialization();
        llvm::Module::FunctionListType &functionList = module->getFunctionList();
        for ( llvm::Module::FunctionListType::iterator it=functionList.begin(); it!=functionList.end(); ++it )
          functionPassManager->run( *it );
        delete functionPassManager;
      }
      
      if ( runFlags & RF_ShowOptIR )
      {
        if ( runFlags & RF_Verbose )
          printf( "-- Optimized IR --\n" ); 
        std::string irString;
        llvm::raw_string_ostream byteCodeStream( irString );
        module->print( byteCodeStream, 0 );
        byteCodeStream.flush();
        printf( "%s", irString.c_str() );
      }

      if ( runFlags & RF_Run )
      {
        if ( runFlags & RF_Verbose )
          printf( "-- Run --\n" );
          
        std::string symbolName;
        std::vector< RC::ConstHandle<AST::FunctionBase> > functionBases;
        globalList->collectFunctionBases( functionBases );
        for ( std::vector< RC::ConstHandle<AST::FunctionBase> >::const_iterator it=functionBases.begin(); it!=functionBases.end(); ++it )
        {
          RC::ConstHandle<AST::FunctionBase> const &functionBase = *it;
          
          if ( functionBase->isFunction() )
          {
            RC::ConstHandle<AST::Function> function = RC::ConstHandle<AST::Function>::StaticCast( functionBase );
            
            std::string const &declaredName = function->getDeclaredName();
            if ( declaredName == "entry" )
            {
              if ( !function->isOperator() )
                throw Exception( "entry: must be an operator" );
              symbolName = function->getSymbolName( cgManager );
            }
          }
        }
        if ( symbolName.empty() )
          throw Exception("missing function 'entry'");
        llvm::Function *llvmEntry = module->getFunction( symbolName );
        FABRIC_ASSERT( llvmEntry );

        void (*entryPtr)() = (void (*)())executionEngine->getPointerToFunction( llvmEntry );
        if ( !entryPtr )
          throw Exception("unable to get pointer to entry");
        try
        {
          entryPtr();
        }
        catch ( Fabric::Exception e )
        {
          printf( "Caught exception: %s\n", (const char *)e );
        }
        catch ( ... )
        {
          printf( "Caught generic exception\n" );
        }
      }

      module.release();
      delete executionEngine;
    }
  }
}

int main( int argc, char **argv )
{
  unsigned int runFlags = 0;
  
  while ( true )
  {
    static struct option longOptions[] =
    {
      { "ast", 0, NULL, 'a' },
      { "asm", 0, NULL, 'm' },
      { "bison", 0, NULL, 'b' },
      { "ir", 0, NULL, 'i' },
      { "optasm", 0, NULL, 'p' },
      { "optir", 0, NULL, 'o' },
      { "run", 0, NULL, 'r' },
      { "tokens", 0, NULL, 't' },
      { "verbose", 0, NULL, 'v' },
      { "guarded", 0, NULL, 'g' },
      { "unguarded", 0, NULL, 'u' },
      { NULL, 0, NULL, 0 }
    };
    int option_index = 0;

    int c = getopt_long( argc, argv, "", longOptions, &option_index );
    if ( c == -1 )
      break;
    switch ( c )
    {
      case 'a':
        runFlags |= RF_ShowAST;
        break;

      case 'b':
        runFlags |= RF_ShowBison;
        
      case 'i':
        runFlags |= RF_ShowIR;
        break;
        
      case 'm':
        runFlags |= RF_ShowASM;
        break;

      case 'o':
        runFlags |= RF_ShowOptIR;
        break;
        
      case 'p':
        runFlags |= RF_ShowOptASM;
        break;
        
      case 'r':
        runFlags |= RF_Run;
        break;
      
      case 't':
        runFlags |= RF_ShowTokens;
        break;
      
      case 'v':
        runFlags |= RF_Verbose;
        break;
      
      case 'u':
        compileOptions.setGuarded( false );
        break;
      
      case 'g':
        compileOptions.setGuarded( true );
        break;
        
      default:
        break;
    }
  }

  if( !( runFlags & ~RF_Verbose ) )
    runFlags |= RF_Run;

  if ( optind == argc )
  {
    try
    {
      handleFile( "(stdin)", stdin, runFlags );
    }
    catch ( Exception e )
    {
      fprintf( stderr, "Caught Exception '%s'\n", (const char*)e.getDesc() );
    }
  }
  else
  {
    for ( int i=optind; i<argc; ++i )
    {
      FILE *fp = fopen( argv[i], "r" );
      if ( !fp )
        perror( argv[i] );
      else
      {
        if ( argc - optind >= 2 )
          printf( "--- %s ---\n", argv[i] );
        try
        {
          handleFile( argv[i], fp, runFlags );
        }
        catch ( Exception e )
        {
          fprintf( stderr, "Caught Exception '%s'\n", (const char*)e.getDesc() );
        }
        fclose( fp );
      }
    }
  }

#if defined( FABRIC_OS_WINDOWS )
  // Stupid hack so that we don't lose the shell window in VS debugger
  if( ::IsDebuggerPresent() )
    getchar();
#endif

  return 0;
}
