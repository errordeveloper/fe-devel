/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "ArrayImpl.h"

#include <Fabric/Base/JSON/Array.h>
#include <Fabric/Core/Util/Encoder.h>
#include <Fabric/Core/Util/Decoder.h>

namespace Fabric
{
  namespace RT
  {
    std::string ArrayImpl::descData( void const *data ) const
    {
      std::string result = "[";
      size_t numMembers = getNumMembers( data );
      size_t numMembersToDisplay = numMembers;
      if ( numMembersToDisplay > 4 )
        numMembersToDisplay = 4;
      for ( size_t i=0; i<numMembersToDisplay; ++i )
      {
        if ( result.length() > 1 )
          result += ",";
        result += m_memberImpl->descData( getMemberData( data, i ) );
      }
      if ( numMembers > numMembersToDisplay )
        result += "...";
      result += "]";
      return result;
    }
    
    bool ArrayImpl::equalsData( void const *lhs, void const *rhs ) const
    {
      size_t lhsSize = getNumMembers( lhs );
      size_t rhsSize = getNumMembers( rhs );
      if ( lhsSize != rhsSize )
        return false;
      else if( lhsSize )
      {
        if( m_memberIsShallow )
          return memcmp( getMemberData( lhs, 0 ), getMemberData( rhs, 0 ), lhsSize * m_memberImpl->getAllocSize() ) == 0;
        else
        {
          for ( size_t i=0; i<lhsSize; ++i )
          {
            if( !m_memberImpl->equalsData( getMemberData( lhs, i ), getMemberData( rhs, i ) ) )
              return false;
          }
          return true;
        }
      }
      else
        return true;
    }
  };
};
