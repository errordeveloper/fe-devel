/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/CG/PencilSymbol.h>

namespace Fabric
{
  namespace CG
  {
    RC::Handle<PencilSymbol> PencilSymbol::Create()
    {
      return new PencilSymbol();
    }
    
    PencilSymbol::PencilSymbol()
    {
    }
          
    bool PencilSymbol::isPencil() const
    {
      return true;
    }
      
    Function const *PencilSymbol::maybeGetPreciseFunction( ExprTypeVector const &argTypes ) const
    {
      for ( FunctionVector::const_iterator it=m_functions.begin(); it!=m_functions.end(); ++it )
      {
        if ( it->isExactMatch( argTypes ) )
        {
          return &*it;
        }
      }
        
      return 0;
    }
      
    void PencilSymbol::add( Function const &function )
    {
      ExprTypeVector argTypes;
      function.appendParamTypes( argTypes );
      FABRIC_ASSERT( !maybeGetPreciseFunction( argTypes ) );
      m_functions.push_back( function );
    }
    
    static Function const *ResolveCandidates(
      Location const &location,
      std::string const &queryDesc,
      std::vector<Function const *> const &candidates
      )
    {
      if ( candidates.size() == 1 )
        return candidates[0];
      else if ( candidates.size() > 1 )
      {
        std::string message = "ambiguous call for " + queryDesc;
        message += "\ncandidates are:";
        for ( std::vector<Function const *>::const_iterator it=candidates.begin(); it!=candidates.end(); ++it )
          message += "\n  " + (*it)->getDesc();
        throw CG::Error( location, message );
      }
      else return 0;
    }
    
    Function const *PencilSymbol::maybeGetFunction(
      Location const &location,
      ModuleBuilder const &moduleBuilder,
      ExprTypeVector const &argTypes,
      std::string const &queryDesc
      ) const
    {
      Function const *function = maybeGetPreciseFunction( argTypes );
      
      if ( !function )
      {
        std::vector<Function const *> candidates;
        
        for ( FunctionVector::const_iterator it=m_functions.begin(); it!=m_functions.end(); ++it )
        {
          if ( it->isLValueToRValueMatch( argTypes ) )
            candidates.push_back( &*it );
        }
        
        function = ResolveCandidates(
          location,
          queryDesc,
          candidates
          );
      }
      
      if ( !function )
      {
        size_t lowestCost = SIZE_MAX;
        std::vector<Function const *> candidates;
        
        for ( FunctionVector::const_iterator it=m_functions.begin(); it!=m_functions.end(); ++it )
        {
          size_t thisCost;
          if ( it->isImplicitCastMatch( argTypes, moduleBuilder, thisCost ) )
          {
            if ( thisCost < lowestCost )
            {
              candidates.clear();
              lowestCost = thisCost;
            }
            if ( thisCost == lowestCost )
              candidates.push_back( &*it );
          }
        }
        
        function = ResolveCandidates(
          location,
          queryDesc,
          candidates
          );
      }
        
      return function;
    }
    
    Function const *PencilSymbol::getFunction(
      Location const &location,
      ModuleBuilder const &moduleBuilder,
      ExprTypeVector const &argTypes,
      std::string const &queryDesc
      ) const
    {
      Function const *result = maybeGetFunction( location, moduleBuilder, argTypes, queryDesc );
      if ( !result )
        throw CG::Error( location, "no definition found for " + queryDesc );
      return result;
    }
      
    Function const *PencilSymbol::getUniqueFunction(
      Location const &location,
      std::string const &queryDesc
      ) const
    {
      if ( m_functions.empty() )
        throw CG::Error( location, "no definition found for " + queryDesc );
      
      if ( m_functions.size() > 1 )
        throw CG::Error( location, queryDesc + " is not unique" );
      
      return &m_functions[0];
    }
  }
}
