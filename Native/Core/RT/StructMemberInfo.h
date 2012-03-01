/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_STRUCT_MEMBER_INFO_H
#define _FABRIC_RT_STRUCT_MEMBER_INFO_H

#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Core/RT/Desc.h>

#include <string>
#include <stdint.h>
#include <vector>

namespace Fabric
{
  namespace RT
  {
    struct StructMemberInfo
    {
      RC::ConstHandle<Desc> desc;
      std::string name;
      std::vector<uint8_t> defaultData;
    };
    
    typedef std::vector<StructMemberInfo> StructMemberInfoVector;
  };
};

#endif //_FABRIC_RT_STRUCT_MEMBER_INFO_H
