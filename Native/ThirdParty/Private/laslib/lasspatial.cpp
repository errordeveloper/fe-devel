/*
===============================================================================

  FILE:  lasspatial.cpp
  
  CONTENTS:
  
    see corresponding header file
  
  PROGRAMMERS:
  
    martin.isenburg@gmail.com
  
  COPYRIGHT:
  
    (c) 2011, Martin Isenburg, LASSO - tools to catch reality

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    see corresponding header file
  
===============================================================================
*/
#include "lasspatial.hpp"

#include "bytestreamin.hpp"
#include "bytestreamout.hpp"

#include "lasquadtree.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LASspatial* LASspatialReadWrite::read(ByteStreamIn* stream) const
{
  char signature[4];
  if (!stream->getBytes((U8*)signature, 4))
  {
    fprintf(stderr,"ERROR (LASspatialReadWrite): reading signature\n");
    return FALSE;
  }
  U32 type;
  if (!stream->getBytes((U8*)&type, 4))
  {
    fprintf(stderr,"ERROR (LASspatialReadWrite): reading type\n");
    return 0;
  }
  LASspatial* spatial;
  if (type == LAS_SPATIAL_QUAD_TREE)
  {
    spatial = new LASquadtree;
    if (!spatial->read(stream))
    {
      delete spatial;
      return 0;
    }
    return spatial;
  }
  else
  {
    fprintf(stderr,"ERROR (LASspatialReadWrite): unknown type %u\n", type);
    return 0;
  }
  return spatial;
}

BOOL LASspatialReadWrite::write(const LASspatial* spatial, ByteStreamOut* stream) const
{
  if (!stream->putBytes((U8*)"LASS", 4))
  {
    fprintf(stderr,"ERROR (LASspatialReadWrite): writing signature\n");
    return FALSE;
  }
  U32 type = LAS_SPATIAL_QUAD_TREE;
  if (!stream->put32bitsLE((U8*)&type))
  {
    fprintf(stderr,"ERROR (LASspatialReadWrite): writing type %u\n", type);
    return FALSE;
  }
  return spatial->write(stream);
}
