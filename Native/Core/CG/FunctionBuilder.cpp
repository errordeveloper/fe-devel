/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "FunctionBuilder.h"
#include "ModuleBuilder.h"
#include "Manager.h"
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Core/RT/Manager.h>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Analysis/Verifier.h>

namespace Fabric
{
  namespace CG
  {
    FunctionBuilder::FunctionBuilder(
      ModuleBuilder &moduleBuilder, 
      std::string const &pencilKey, 
      std::string const &symbolName,
      std::string const &desc,
      RC::ConstHandle<Adapter> const &returnAdapter, 
      ParamVector const &params,
      size_t flags,
      PolymorphismParameters const &polymorphismParameters
      )
      : m_moduleBuilder( moduleBuilder )
      , m_functionScope( NULL )
    {
      build( pencilKey, symbolName, desc, returnAdapter, params, flags, polymorphismParameters );
    }
    
    FunctionBuilder::FunctionBuilder( 
      ModuleBuilder &moduleBuilder, 
      std::string const &pencilKey, 
      std::string const &symbolName,
      std::string const &desc,
      std::string const &paramLayout,
      size_t flags
      )
      : m_moduleBuilder( moduleBuilder )
      , m_functionScope( NULL )
    {
      RC::Handle<CG::Manager> manager = moduleBuilder.getManager();
      CG::ParamVector paramList;
      RC::ConstHandle<Adapter> returnAdapter;
      
      if( !paramLayout.empty() )
      {
          // The parameter layout is defined thus "{=,<,>}[Type]:[Name]"
          // E.g.: "=Scalar,<Integer:num,>Integer:tex"
          // corresponds to the following KL function:
          /// Scalar foo( in Integer num, io/out Integer tex );
        
        size_t          start, end;
        start = 0;
        
        do
        {
          end = paramLayout.find( ',', start + 1 );
          
          CG::Usage usage = CG::USAGE_UNSPECIFIED;
          if ( paramLayout[ start ] == '<' )
            usage = CG::USAGE_RVALUE;
          else if ( paramLayout[ start ] == '>' )
            usage = CG::USAGE_LVALUE;
          else if ( paramLayout[ start ] == '=' )
          {
            if ( returnAdapter )
              throw Exception( "Malformed function param layout string. Return type already specified: " + paramLayout );
            
            std::string rtype = paramLayout.substr( start + 1, end - start - 1 );
            RC::ConstHandle<RT::Desc> typeDesc = manager->getRTManager()->getDesc( rtype );
            returnAdapter = manager->getAdapter( typeDesc );
          }
          else
            throw Exception( "Malformed function param layout string. Missing in/out marker: " + paramLayout.substr( start, end - start ) );
          
          if ( usage != CG::USAGE_UNSPECIFIED )
          {
            size_t split = paramLayout.find( ' ', start + 2 );
            if ( split > end )
              throw Exception( "Malformed function param layout string. Missing type: " + paramLayout.substr( start, end - start ) );
            
            std::string ptype = paramLayout.substr( start + 1, split - start - 1 );
            RC::ConstHandle<RT::Desc> typeDesc = manager->getRTManager()->getDesc( ptype );
            RC::ConstHandle<CG::Adapter> typeAdapter = manager->getAdapter( typeDesc );
            
            std::string pname = paramLayout.substr( split + 1, end - split - 1 );
            if ( pname.empty() )
              throw Exception( "Malformed function param layout string. Missing param name: " + paramLayout.substr( start, end - start ) );
            
            paramList.push_back( CG::FunctionParam( pname, typeAdapter, usage ) );
          }
          
          start = end + 1;
        } while( end != std::string::npos );
      }
      
      build( pencilKey, symbolName, desc, returnAdapter, paramList, flags );
    }
    
    void FunctionBuilder::build( 
      std::string const &pencilKey,
      std::string const &symbolName, 
      std::string const &desc,
      RC::ConstHandle<Adapter> const &returnAdapter, 
      ParamVector const &params, 
      size_t flags,
      PolymorphismParameters const &polymorphismParameters
      )
    {
      RC::Handle<Context> context = getContext();
      
      llvm::Type const *llvmReturnType = 0;
      std::vector<llvm::Type const *> llvmParamTypes;

      if ( flags & DirectlyReturnLValue )
      {
        m_haveHiddenReturnLValue = false;
        llvmReturnType = returnAdapter->llvmLType( context );
      }
      else if ( flags & DirectlyReturnRValue )
      {
        m_haveHiddenReturnLValue = false;
        llvmReturnType = returnAdapter->llvmRType( context );
      }
      else if ( returnAdapter )
      {
        if ( returnAdapter->usesReturnLValue() )
        {
          m_haveHiddenReturnLValue = true;
          llvmReturnType = llvm::Type::getVoidTy( context->getLLVMContext() );
          llvmParamTypes.push_back( returnAdapter->llvmLType( context ) );
        }
        else
        {
          m_haveHiddenReturnLValue = false;
          llvmReturnType = returnAdapter->llvmRType( context );
        }
      }
      else
      {
        m_haveHiddenReturnLValue = false;
        llvmReturnType = llvm::Type::getVoidTy( context->getLLVMContext() );
      }

      for ( size_t i=0; i<params.size(); ++i )
        llvmParamTypes.push_back( params[i].getLLVMType( context ) );
        
      m_llvmFunctionType = llvm::FunctionType::get( llvmReturnType, llvmParamTypes, false );

      llvm::AttributeWithIndex AWI[1];
      AWI[0] = llvm::AttributeWithIndex::get( ~0u, llvm::Attribute::InlineHint );
      llvm::AttrListPtr attrListPtr = llvm::AttrListPtr::get(AWI, 1);
      
      llvm::Function *existingLLVMFunction = m_moduleBuilder->getFunction( symbolName.c_str() );
      if ( existingLLVMFunction )
      {
        // [pzion 20110825] FIXME: should verify sanity here through asserts on types, attributes
        // and linkage
        m_llvmFunction = existingLLVMFunction;
      }
      else
      {
        m_llvmFunction = llvm::cast<llvm::Function>( m_moduleBuilder->getOrInsertFunction( symbolName.c_str(), m_llvmFunctionType, attrListPtr ) );
        m_llvmFunction->setLinkage( (flags & ExportSymbol)? llvm::GlobalValue::ExternalLinkage: llvm::GlobalValue::PrivateLinkage );
      }
    
      llvm::Function::arg_iterator ai = m_llvmFunction->arg_begin();
      
      ReturnInfo returnInfo( ExprType( returnAdapter, (flags & DirectlyReturnLValue)? USAGE_LVALUE: USAGE_RVALUE ) );
      if ( m_haveHiddenReturnLValue )
      {
        ai->setName( "resultLValue" );
        ai->addAttr( llvm::Attribute::NoCapture );
        ai->addAttr( llvm::Attribute::NoAlias );
        ai->addAttr( llvm::Attribute::StructRet );
        returnInfo = ReturnInfo( ExprType( returnAdapter, USAGE_LVALUE ), ai );
        ++ai;
      }

      m_functionScope = new FunctionScope( m_moduleBuilder.getScope(), returnInfo );

      bool haveContainer = false;
      llvm::Function::arg_iterator savedAI = ai;
      for ( size_t i=0; i<params.size(); ++i, ++ai )
      {
        FunctionParam const &param = params[i];
        if ( param.getUsage() == USAGE_LVALUE
          && param.getAdapter()->getDesc()->isNoAliasSafe() )
        {
          haveContainer = true;
          break;
        }
      }
      ai = savedAI;
      
      for ( size_t i=0; i<params.size(); ++i, ++ai )
      {
        FunctionParam const &param = params[i];
        ai->setName( param.getName() );
        if ( param.getUsage() == USAGE_LVALUE
          || param.getAdapter()->isPassByReference()  )
        {
          ai->addAttr( llvm::Attribute::NoCapture );
          if ( !haveContainer
            || ( !RT::isSlicedArray( param.getAdapter()->getType() )
              && param.getAdapter()->getDesc()->isNoAliasSafe() ) )
            ai->addAttr( llvm::Attribute::NoAlias );
        }
        m_functionScope->put( param.getName(), ParameterSymbol::Create( CG::ExprValue( param.getExprType(), context, ai ) ) );
      }
      
      if ( !pencilKey.empty() )
        m_pencil = m_moduleBuilder.addFunction(
          pencilKey,
          Function(
            desc,
            m_llvmFunction,
            returnInfo,
            params,
            polymorphismParameters
            )
          );
    }

    FunctionBuilder::FunctionBuilder(
      ModuleBuilder &moduleBuilder,
      llvm::FunctionType const *llvmFunctionType,
      llvm::Function *llvmFunction
      )
      : m_moduleBuilder( moduleBuilder )
      , m_llvmFunctionType( llvmFunctionType )
      , m_llvmFunction( llvmFunction )
      , m_haveHiddenReturnLValue( false )
    {
      ReturnInfo returnInfo( ExprType(), 0 );
      m_functionScope = new FunctionScope( m_moduleBuilder.getScope(), returnInfo );
    }
    
    ModuleBuilder &FunctionBuilder::getModuleBuilder()
    {
      return m_moduleBuilder;
    }
    
    RC::Handle<Manager> FunctionBuilder::getManager()
    {
      return m_moduleBuilder.getManager();
    }
    
    RC::Handle<Context> FunctionBuilder::getContext()
    {
      return m_moduleBuilder.getContext();
    }
    
    llvm::Function *FunctionBuilder::operator ->()
    {
      return m_llvmFunction;
    }
    
    llvm::Function *FunctionBuilder::getLLVMFunction()
    {
      return m_llvmFunction;
    }
    
    FunctionBuilder::~FunctionBuilder()
    {
      delete m_functionScope;
    }
      
    llvm::Argument *FunctionBuilder::operator[]( size_t index )
    {
      llvm::Function::ArgumentListType &argumentList = m_llvmFunction->getArgumentList();
      llvm::Function::ArgumentListType::iterator it = argumentList.begin();
      
      // Skip the 'hidden' return value.
      if ( m_haveHiddenReturnLValue )
        ++it;

      while ( index-- )
        ++it;
      return it;
    }

    llvm::BasicBlock *FunctionBuilder::createBasicBlock( std::string const &name )
    {
      return llvm::BasicBlock::Create( getContext()->getLLVMContext(), name, m_llvmFunction );
    }
      
    FunctionScope &FunctionBuilder::getScope()
    {
      FABRIC_ASSERT( m_functionScope );
      return *m_functionScope;
    }
    
    RC::ConstHandle<Adapter> FunctionBuilder::maybeGetAdapter( std::string const &userName ) const
    {
      return m_moduleBuilder.maybeGetAdapter( userName );
    }
    
    RC::ConstHandle<Adapter> FunctionBuilder::getAdapter( std::string const &userName, CG::Location const &location ) const
    {
      return m_moduleBuilder.getAdapter( userName, location );
    }

    FunctionBuilder::FunctionBuilder(
      ModuleBuilder &moduleBuilder, 
      RC::ConstHandle<CG::Adapter> const &returnAdapter,
      std::string const &functionName,
      std::string const &param1Name,
      RC::ConstHandle<CG::Adapter> const &param1Adapter,
      Usage param1Usage,
      size_t flags
      )
      : m_moduleBuilder( moduleBuilder )
      , m_functionScope( NULL )
    {
      ParamVector params;
      params.push_back( FriendlyFunctionParam( param1Name, param1Adapter, param1Usage ) );
      std::string pencilKey = FunctionPencilKey( functionName );
      std::string symbolName = FunctionDefaultSymbolName( functionName, params.getTypes() );
      std::string desc = FunctionFullDesc( returnAdapter, functionName, params.getTypes() );
      build( pencilKey, symbolName, desc, returnAdapter, params, flags );
      getModuleBuilder().getScope().put( functionName, getPencil() );
    }

    FunctionBuilder::FunctionBuilder(
      ModuleBuilder &moduleBuilder, 
      RC::ConstHandle<CG::Adapter> const &returnAdapter,
      std::string const &functionName,
      std::string const &param1Name,
      RC::ConstHandle<CG::Adapter> const &param1Adapter,
      Usage param1Usage,
      std::string const &param2Name,
      RC::ConstHandle<CG::Adapter> const &param2Adapter,
      Usage param2Usage,
      size_t flags
      )
      : m_moduleBuilder( moduleBuilder )
      , m_functionScope( NULL )
    {
      ParamVector params;
      params.push_back( FriendlyFunctionParam( param1Name, param1Adapter, param1Usage ) );
      params.push_back( FriendlyFunctionParam( param2Name, param2Adapter, param2Usage ) );
      std::string pencilKey = FunctionPencilKey( functionName );
      std::string symbolName = FunctionDefaultSymbolName( functionName, params.getTypes() );
      std::string desc = FunctionFullDesc( returnAdapter, functionName, params.getTypes() );
      build( pencilKey, symbolName, desc, returnAdapter, params, flags );
      getModuleBuilder().getScope().put( functionName, getPencil() );
    }
    
    RC::ConstHandle<PencilSymbol> FunctionBuilder::getPencil() const
    {
      return m_pencil;
    }
  }
}
