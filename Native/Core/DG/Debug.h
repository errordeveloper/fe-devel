/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_DEBUG_H
#define _FABRIC_DG_DEBUG_H
 
#include <Fabric/Core/Util/Debug.h>

#if defined(FABRIC_BUILD_DEBUG)
namespace Fabric
{
  namespace DG
  {
    extern bool gDebugEnabled;
  };
};
#endif

#define FABRIC_DG_TRACE _FABRIC_TRACE( DG )
#define FABRIC_DG_TRACE_NOTE _FABRIC_TRACE_NOTE
#define FABRIC_DG_TRACE_DUMP_BUFFER( size, data ) _FABRIC_TRACE_DUMP_BUFFER( size, data )

#endif //_FABRIC_DG_DEBUG_H
