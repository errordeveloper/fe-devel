/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/CG/Error.h>

namespace Fabric
{
  namespace CG
  {
    Error::Error( Location const &location, Util::SimpleString const &desc )
      : Exception( desc )
      , m_location( location )
    {
    }
    
    Error::Error( Error const &that )
      : Exception( that )
      , m_location( that.m_location )
    {
    }
    
    Error &Error::operator =( Error const &that )
    {
      *(Exception *)this = that;
      m_location = that.m_location;
      return *this;
    }
    
    Error operator +( Util::SimpleString const &prefix, Error const &error )
    {
      return Error( error.getLocation(), prefix + error.getDesc() );
    }

    Error operator +( char const *cString, Error const &error )
    {
      return Error( error.getLocation(), cString + error.getDesc() );
    }
  };
};
