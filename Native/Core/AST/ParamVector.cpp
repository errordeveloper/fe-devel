/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/AST/Param.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    RC::ConstHandle<ParamVector> ParamVector::Create( RC::ConstHandle<Param> const &firstParam, RC::ConstHandle<Param> const &secondParam )
    {
      ParamVector *result = new ParamVector;
      result->push_back( firstParam );
      result->push_back( secondParam );
      return result;
    }
    
    RC::ConstHandle<ParamVector> ParamVector::Create( RC::ConstHandle<Param> const &firstParam, RC::ConstHandle<ParamVector> const &remainingParams )
    {
      ParamVector *result = new ParamVector;
      if ( firstParam )
        result->push_back( firstParam );
      if ( remainingParams )
      {
        for ( size_t i=0; i<remainingParams->size(); ++i )
          result->push_back( remainingParams->get(i) );
      }
      return result;
    }
    
    ParamVector::ParamVector()
    {
    }
    
    void ParamVector::appendJSON( Util::JSONGenerator const &jsonGenerator, bool includeLocation ) const
    {
      Util::JSONArrayGenerator jsonArrayGenerator = jsonGenerator.makeArray();
      for ( const_iterator it=begin(); it!=end(); ++it )
        (*it)->appendJSON( jsonArrayGenerator.makeElement(), includeLocation );
    }
      
    std::vector<CG::FunctionParam> ParamVector::getFunctionParams( RC::Handle<CG::Manager> const &cgManager ) const
    {
      std::vector<CG::FunctionParam> result;
      for ( size_t i=0; i<size(); ++i )
        result.push_back( get(i)->getFunctionParam( cgManager ) );
      return result;
    }
    
    std::vector<std::string> ParamVector::getTypes() const
    {
      std::vector<std::string> result;
      for ( size_t i=0; i<size(); ++i )
        result.push_back( get(i)->getType() );
      return result;
    }
    
    std::vector< RC::ConstHandle<CG::Adapter> > ParamVector::getAdapters( RC::Handle<CG::Manager> const &cgManager ) const
    {
      std::vector< RC::ConstHandle<CG::Adapter> > result;
      for ( size_t i=0; i<size(); ++i )
        result.push_back( get(i)->getAdapter( cgManager ) );
      return result;
    }
    
    std::vector<CG::ExprType> ParamVector::getExprTypes( RC::Handle<CG::Manager> const &cgManager ) const
    {
      std::vector<CG::ExprType> result;
      for ( size_t i=0; i<size(); ++i )
        result.push_back( get(i)->getExprType( cgManager ) );
      return result;
    }
    
    void ParamVector::registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      for ( const_iterator it=begin(); it!=end(); ++it )
        (*it)->registerTypes( cgManager, diagnostics );
    }

    void ParamVector::llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const
    {
      for ( const_iterator it=begin(); it!=end(); ++it )
        (*it)->llvmCompileToModule( moduleBuilder, diagnostics, buildFunctionBodies );
    }
  };
};
