/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/RequireVector.h>
#include <Fabric/Core/AST/Require.h>

namespace Fabric
{
  namespace AST
  {
    RC::ConstHandle<RequireVector> RequireVector::Create( RC::ConstHandle<Require> const &first, RC::ConstHandle<RequireVector> const &remaining )
    {
      RequireVector *result = new RequireVector;
      if ( first )
        result->push_back( first );
      if ( remaining )
      {
        for ( RequireVector::const_iterator it=remaining->begin(); it!=remaining->end(); ++it )
          result->push_back( *it );
      }
      return result;
    }
    
    RequireVector::RequireVector()
    {
    }
    
    void RequireVector::appendJSON( JSON::Encoder const &encoder, bool includeLocation ) const
    {
      JSON::ArrayEncoder jsonArrayEncoder = encoder.makeArray();
      for ( const_iterator it=begin(); it!=end(); ++it )
        (*it)->appendJSON( jsonArrayEncoder.makeElement(), includeLocation );
    }
    
    void RequireVector::collectRequires( RequireNameToLocationMap &uses ) const
    {
      for ( const_iterator it=begin(); it!=end(); ++it )
        (*it)->collectRequires( uses );
    }
  }
}
