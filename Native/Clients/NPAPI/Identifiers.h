/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_NPAPI_IDENTIFIERS_H
#define _FABRIC_NPAPI_IDENTIFIERS_H

#include <npapi/npapi.h>
#include <npapi/npruntime.h>
#include <string>
#include <stddef.h>

namespace Fabric
{
  namespace NPAPI
  {
    std::string const *identifierToString( NPIdentifier identifier );
    
    NPIdentifier stringToIdentifier( std::string const &string );
    NPIdentifier integerToIdentifier( size_t integer );
  };
};

#endif //_FABRIC_NPAPI_IDENTIFIERS_H

