/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_ERROR_H
#define _FABRIC_CG_ERROR_H

#include <Fabric/Base/Exception.h>
#include <Fabric/Core/CG/Location.h>

namespace Fabric
{
  namespace CG
  {
    class Error : public Exception
    {
    public:
    
      Error( Location const &location, Util::SimpleString const &desc );
      Error( Error const &that );
      
      Error &operator =( Error const &that );
      
      Location const &getLocation() const
      {
        return m_location;
      }
      
    private:
    
      Location m_location;
    };
    
    Error operator +( Util::SimpleString const &prefix, Error const &error );
    Error operator +( char const *cString, Error const &error );
  };
};

#endif //_FABRIC_CG_ERROR_H
