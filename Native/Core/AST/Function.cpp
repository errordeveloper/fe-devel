/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "Function.h"
#include <Fabric/Base/JSON/String.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( Function );
    
    RC::Handle<Function> Function::Create(
      CG::Location const &location,
      std::string const &friendlyName,
      std::string const &entryName,
      std::string const &returnTypeName,
      RC::ConstHandle<ParamVector> const &params,
      RC::ConstHandle<CompoundStatement> const &body
      )
    {
      return new Function( location, friendlyName, entryName, returnTypeName, params, body );
    }
    
    Function::Function(
        CG::Location const &location,
        std::string const &friendlyName,
        std::string const &entryName,
        std::string const &returnTypeName,
        RC::ConstHandle<ParamVector> const &params,
        RC::ConstHandle<CompoundStatement> const &body
        )
      : FunctionBase( location, returnTypeName, params, body )
      , m_friendlyName( friendlyName )
      , m_entryName( entryName )
    {
    }
    
    RC::Handle<JSON::Object> Function::toJSON() const
    {
      RC::Handle<JSON::Object> result = FunctionBase::toJSON();
      result->set( "friendlyName", JSON::String::Create( m_friendlyName ) );
      result->set( "entryName", JSON::String::Create( m_entryName ) );
      return result;
    }
    
    std::string const *Function::getFriendlyName() const
    {
      return &m_friendlyName;
    }

    std::string Function::getEntryName( RC::Handle<CG::Manager> const &cgManager ) const
    {
      return m_entryName;
    }
  };
};
