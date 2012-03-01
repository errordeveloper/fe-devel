/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/ContainerLoop.h>
#include <Fabric/Core/AST/Expr.h>
#include <Fabric/Core/CG/Context.h>
#include <Fabric/Core/CG/ComparableAdapter.h>
#include <Fabric/Core/CG/DictAdapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Core/CG/Error.h>
#include <Fabric/Core/CG/FunctionBuilder.h>
#include <Fabric/Core/RT/Impl.h>
#include <Fabric/Base/Util/SimpleString.h>

#include <llvm/Function.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( ContainerLoop );
    
    RC::ConstHandle<ContainerLoop> ContainerLoop::Create(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &dictExpr,
      std::string const &keyName,
      std::string const &valueName,
      RC::ConstHandle<Statement> const &body
      )
    {
      return new ContainerLoop(
        location,
        dictExpr,
        keyName,
        valueName,
        body
        );
    }
    
    ContainerLoop::ContainerLoop(
      CG::Location const &location,
      RC::ConstHandle<Expr> const &dictExpr,
      std::string const &keyName,
      std::string const &valueName,
      RC::ConstHandle<Statement> const &body
      )
      : Statement( location )
      , m_dictExpr( dictExpr )
      , m_keyName( keyName )
      , m_valueName( valueName )
      , m_body( body )
    {
    }
    
    void ContainerLoop::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Statement::appendJSONMembers( jsonObjectEncoder, includeLocation );
      m_dictExpr->appendJSON( jsonObjectEncoder.makeMember( "dictExpr" ), includeLocation );
      jsonObjectEncoder.makeMember( "keyName" ).makeString( m_keyName );
      if ( m_valueName.length() > 0 )
        jsonObjectEncoder.makeMember( "valueName" ).makeString( m_valueName );
      m_body->appendJSON( jsonObjectEncoder.makeMember( "body" ), includeLocation );
    }
    
    void ContainerLoop::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      m_dictExpr->registerTypes( cgManager, diagnostics );
      m_body->registerTypes( cgManager, diagnostics );
    }

    void ContainerLoop::llvmCompileToBuilder( CG::BasicBlockBuilder &parentBasicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      try
      {
        CG::BasicBlockBuilder &basicBlockBuilder = parentBasicBlockBuilder;
        
        RC::Handle<CG::Context> context = parentBasicBlockBuilder.getContext();
        
        CG::ExprValue dictExprValue = m_dictExpr->buildExprValue( parentBasicBlockBuilder, CG::USAGE_UNSPECIFIED, "" );
        RC::ConstHandle<CG::Adapter> adapter = dictExprValue.getAdapter();
        if ( !RT::isDict( adapter->getType() ) )
          throw CG::Error( m_dictExpr->getLocation(), "must be a dictionary" );
        RC::ConstHandle<CG::DictAdapter> dictAdapter = RC::ConstHandle<CG::DictAdapter>::StaticCast( adapter );
        RC::ConstHandle<CG::ComparableAdapter> keyAdapter = dictAdapter->getKeyAdapter();
        RC::ConstHandle<CG::Adapter> valueAdapter = dictAdapter->getValueAdapter();

        llvm::Type const *nodePtrType = dictAdapter->getLLVMNodePtrType( context );
        llvm::Instruction *nodePtrPtr = new llvm::AllocaInst( nodePtrType );
        nodePtrPtr->setName( m_keyName );
        
        // [pzion 20111019] We allocate variables at the top of the function
        CG::FunctionBuilder &functionBuilder = parentBasicBlockBuilder.getFunctionBuilder();
        llvm::BasicBlock &entryBB = functionBuilder->getEntryBlock();
        llvm::BasicBlock::iterator it = entryBB.begin();
        while ( it != entryBB.end() )
        {
          if ( !llvm::isa<llvm::AllocaInst>(*it) )
            break;
          ++it;
        }
        entryBB.getInstList().insert( it, nodePtrPtr );
        
        llvm::Value *bitsPtr = dictExprValue.getValue();
        
        llvm::BasicBlock *startBB = parentBasicBlockBuilder.getFunctionBuilder().createBasicBlock( "containerLoopStart" );
        llvm::BasicBlock *checkBB = parentBasicBlockBuilder.getFunctionBuilder().createBasicBlock( "containerLoopCheck" );
        llvm::BasicBlock *bodyBB = parentBasicBlockBuilder.getFunctionBuilder().createBasicBlock( "containerLoopBody" );
        llvm::BasicBlock *stepBB = parentBasicBlockBuilder.getFunctionBuilder().createBasicBlock( "containerLoopStep" );
        llvm::BasicBlock *endBB = parentBasicBlockBuilder.getFunctionBuilder().createBasicBlock( "containerLoopEnd" );
        
        basicBlockBuilder->CreateBr( startBB );
        
        // [pzion 20111019] Load bits->firstNode into llvmNodePtrPtr
        basicBlockBuilder->SetInsertPoint( startBB );
        llvm::Value *firstNodePtrPtr = basicBlockBuilder->CreateStructGEP( bitsPtr, 2 );
        llvm::Value *firstNodePtr = basicBlockBuilder->CreateLoad( firstNodePtrPtr );
        basicBlockBuilder->CreateStore( firstNodePtr, nodePtrPtr );
        basicBlockBuilder->CreateBr( checkBB );

        basicBlockBuilder->SetInsertPoint( checkBB );
        llvm::Value *nodePtr = basicBlockBuilder->CreateLoad( nodePtrPtr );
        basicBlockBuilder->CreateCondBr(
          basicBlockBuilder->CreateIsNull( nodePtr ),
          endBB,
          bodyBB
          );

        basicBlockBuilder->SetInsertPoint( bodyBB );
        llvm::Value *bitsNextNodePtrPtr = basicBlockBuilder->CreateStructGEP( nodePtr, 1 );
        llvm::Value *bitsNextNodePtr = basicBlockBuilder->CreatePointerCast(
          basicBlockBuilder->CreateLoad( bitsNextNodePtrPtr ),
          nodePtrType
          );
        {
          CG::LoopScope loopScope( parentBasicBlockBuilder.getScope(), endBB, stepBB );
          llvm::Value *keyLValue = basicBlockBuilder->CreatePointerCast(
            basicBlockBuilder->CreateStructGEP( nodePtr, 5 ),
            keyAdapter->llvmLType( context )
            );
          llvm::Value *keyRValue = keyAdapter->llvmLValueToRValue( basicBlockBuilder, keyLValue );
          loopScope.put( m_keyName, CG::ParameterSymbol::Create( CG::ExprValue( keyAdapter, CG::USAGE_RVALUE, context, keyRValue ) ) );
          llvm::Value *valueRValue = 0;
          if ( m_valueName.length() > 0 )
          {
            llvm::Value *valueLValue = basicBlockBuilder->CreatePointerCast(
              basicBlockBuilder->CreateConstGEP2_32(
                basicBlockBuilder->CreateStructGEP( nodePtr, 5 ),
                0,
                keyAdapter->getImpl()->getAllocSize()
                ),
              valueAdapter->llvmLType( context )
              );
            switch ( dictExprValue.getUsage() )
            {
              case CG::USAGE_RVALUE:
              {
                valueRValue = valueAdapter->llvmLValueToRValue( basicBlockBuilder, valueLValue );
                loopScope.put( m_valueName, CG::ParameterSymbol::Create( CG::ExprValue( valueAdapter, CG::USAGE_RVALUE, context, valueRValue ) ) );
              }
              break;
              
              case CG::USAGE_LVALUE:
              {
                loopScope.put( m_valueName, CG::ParameterSymbol::Create( CG::ExprValue( valueAdapter, CG::USAGE_LVALUE, context, valueLValue ) ) );
              }
              break;
              
              default:
                FABRIC_ASSERT(false);
                break;
            }
          }
          CG::BasicBlockBuilder loopBasicBlockBuilder( parentBasicBlockBuilder, loopScope );
          m_body->llvmCompileToBuilder( loopBasicBlockBuilder, diagnostics );
          loopScope.llvmUnwind( loopBasicBlockBuilder );
        }
        basicBlockBuilder->CreateBr( stepBB );

        basicBlockBuilder->SetInsertPoint( stepBB );
        basicBlockBuilder->CreateStore( bitsNextNodePtr, nodePtrPtr );
        basicBlockBuilder->CreateBr( checkBB );
        
        basicBlockBuilder->SetInsertPoint( endBB );
      }
      catch ( CG::Error e )
      {
        addError( diagnostics, e );
      }
      catch ( Exception e )
      {
        addError( diagnostics, e );
      }
    }
  };
};
