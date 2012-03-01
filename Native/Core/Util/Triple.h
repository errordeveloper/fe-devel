/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_TRIPLE_H
#define _FABRIC_UTIL_TRIPLE_H

#include <string>

namespace Fabric
{
  namespace Util
  {
    std::string const &getHostTriple();
    
    bool matchHostTriple( std::string const &hostTriple, std::string const &pattern );
  };
};

#endif // _FABRIC_UTIL_TRIPLE_H
