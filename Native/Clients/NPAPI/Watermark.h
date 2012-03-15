/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_NPAPI_WATERMARK_H
#define _FABRIC_NPAPI_WATERMARK_H

#include <stdint.h>
#include <stdlib.h>

namespace Fabric
{
  namespace NPAPI
  {
    static const size_t watermarkWidth = 465;
    static const size_t watermarkHeight = 80;
    extern uint8_t watermarkData[watermarkWidth*watermarkHeight*4];
  };
};

#endif //_FABRIC_NPAPI_WATERMARK_H
