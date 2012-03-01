/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/CG/CompileOptions.h>

namespace Fabric
{
  namespace CG
  {
    void CompileOptions::setFromString( std::string const &string )
    {
      setGuarded( string.find( "guarded" ) < string.length() );
    }
    
    std::string CompileOptions::getString() const
    {
      std::string result = getGuarded()? "guarded": "unguarded";
      return result;
    }
  };
};
