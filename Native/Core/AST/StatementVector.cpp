/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/StatementVector.h>
#include <Fabric/Core/AST/Statement.h>
#include <Fabric/Core/CG/BasicBlockBuilder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    RC::ConstHandle<StatementVector> StatementVector::Create( RC::ConstHandle<Statement> const &first, RC::ConstHandle<StatementVector> const &remaining )
    {
      StatementVector *result = new StatementVector;
      if ( first )
        result->push_back( first );
      if ( remaining )
      {
        for ( StatementVector::const_iterator it=remaining->begin(); it!=remaining->end(); ++it )
          result->push_back( *it );
      }
      return result;
    }
    
    StatementVector::StatementVector()
    {
    }
    
    void StatementVector::appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const
    {
      JSON::ArrayEncoder jsonArrayEncoder = encoder.makeArray();
      for ( const_iterator it=begin(); it!=end(); ++it )
        (*it)->appendJSON( jsonArrayEncoder.makeElement(), includeLocation );
    }
    
    void StatementVector::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      for ( const_iterator it=begin(); it!=end(); ++it )
        (*it)->registerTypes( cgManager, diagnostics );
    }
    
    void StatementVector::llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      for ( size_t i=0; i<size(); ++i )
      {
        RC::ConstHandle<Statement> const &statement = get(i);
        if ( basicBlockBuilder->GetInsertBlock()->getTerminator() )
        {
          statement->addError( diagnostics, "code is unreachable" );
          break;
        }
        statement->llvmCompileToBuilder( basicBlockBuilder, diagnostics );
      }
    }
  };
};
