/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

FABRIC_EXT_EXPORT void exceptThrow(
  KL::String::IN desc
  )
{
  throwException( std::string( desc.data(), desc.length() ) );
}
