/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_KL_SOURCE_H
#define _FABRIC_KL_SOURCE_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/ConstHandle.h>

namespace Fabric
{
  namespace RC
  {
    class String;
  };
  
  namespace KL
  {
    class Source : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
    
      virtual RC::ConstHandle<RC::String> filename() const = 0;
      virtual char const *data() const = 0;
      virtual size_t length() const = 0;
    };
  };
};

#endif //_FABRIC_KL_SOURCE_H
