/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_NPAPI_WATERMARK_H
#define _FABRIC_NPAPI_WATERMARK_H

#include <stdint.h>
#include <stdlib.h>

namespace Fabric
{
  namespace NPAPI
  {
    static const size_t watermarkWidth = 369;
    static const size_t watermarkHeight = 128;
    extern uint8_t watermarkData[watermarkWidth*watermarkHeight*4];
  };
};

#endif //_FABRIC_NPAPI_WATERMARK_H
