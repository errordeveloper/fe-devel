/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>
using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

#include <teem/nrrd.h>
//#include <teem/gage.h>

void FabricTeemNRRDLoadUShortFromFile(
  const KL::String & fileName,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::Size &imageDepth,
  KL::VariableArray<KL::Byte> &imageUShortVoxels,
  KL::Mat44 &xfoMat
  )
{
  Nrrd* nin = nrrdNew();
  if(nrrdLoad(nin, fileName.data(), NULL))
  {
    char * err = biffGetDone(NRRD);
    Fabric::EDK::throwException("FabricTeemNRRDLoadUShort: Exception caught: %s",err);
  }

  if(nin->dim != 3)
    Fabric::EDK::throwException("FabricTeemNRRDLoadUShort: only images of 3 dimensions are supported");

  if(nin->type != nrrdTypeUShort)
    Fabric::EDK::throwException("FabricTeemNRRDLoadUShort: only images of UShort precision are supported");

  size_t i;
  for( i = 0; i < 3; ++i ) {
    unsigned int s = nin->axis[i].size;
    while(s) {
      if((s & 1) && (s>>1) != 0)
        throw "FabricTeemNRRDLoadUShort: only images with power of 2 sizes are supported";
      s = s >> 1;
    }
  }

  size_t maxDimSize = 0;
  for( i = 0; i < 3; ++i ) {
    if(nin->axis[i].size > maxDimSize)
      maxDimSize = nin->axis[i].size;
  }
  float factors[3];
  for( i = 0; i < 3; ++i ) {
    factors[i] = float(nin->axis[i].size) / float(maxDimSize);
  }

  //Flip X and Y axis; seems to be flipped in example files
  xfoMat.row0.x = factors[0]*(float)nin->axis[0].spaceDirection[0]; xfoMat.row0.y = factors[1]*(float)nin->axis[1].spaceDirection[0]; xfoMat.row0.z = factors[2]*(float)nin->axis[2].spaceDirection[0]; xfoMat.row0.t = 0.0;
  xfoMat.row1.x = factors[0]*(float)nin->axis[0].spaceDirection[2]; xfoMat.row1.y = factors[1]*(float)nin->axis[1].spaceDirection[2]; xfoMat.row1.z = factors[2]*(float)nin->axis[2].spaceDirection[2]; xfoMat.row1.t = 0.0;
  xfoMat.row2.x = -factors[0]*(float)nin->axis[0].spaceDirection[1]; xfoMat.row2.y = -factors[1]*(float)nin->axis[1].spaceDirection[1]; xfoMat.row2.z = -factors[2]*(float)nin->axis[2].spaceDirection[1]; xfoMat.row2.t = 0.0;
  xfoMat.row3.x = 0.0; xfoMat.row3.y = 0.0; xfoMat.row3.z = 0.0; xfoMat.row3.t = 1.0;

  imageWidth = nin->axis[0].size;
  imageHeight = nin->axis[1].size;
  imageDepth = nin->axis[2].size;

  imageUShortVoxels.resize( imageWidth * imageHeight * imageDepth * 2 );
  ::memcpy( &(imageUShortVoxels[0]), nin->data, imageUShortVoxels.size() );

/*  

  ////////////////////////////////////////////////////
  //gradientcompute
  //From nrrd gage example: http://teem.sourceforge.net/gage/index.html

  gageContext *ctx;
  gagePerVolume *pvl;
  const double *grad;
  double kparm[NRRD_KERNEL_PARMS_NUM];
  int E;

  if (gageKindVolumeCheck(gageKindScl, nin)) {
    fprintf(stderr, "didn't get a %s volume:\n%s\n",
            gageKindScl->name, biffGetDone(GAGE));
    return 1;
  }

  //Bicubic kernel params. Not sure what B and C are exactly.
  kparm[0] = 1.0; // scale parameter, in units of samples
  kparm[1] = 0.0; // B
  kparm[2] = 0.5; // C

  ctx = gageContextNew();
  E = 0;
  if (!E) E |= !(pvl = gagePerVolumeNew(ctx, nin, gageKindScl));
  if (!E) E |= gagePerVolumeAttach(ctx, pvl);
  if (!E) E |= gageKernelSet(ctx, gageKernel00, nrrdKernelBCCubic, kparm);
  if (!E) E |= gageKernelSet(ctx, gageKernel11, nrrdKernelBCCubicD, kparm);
  if (!E) E |= gageQueryItemOn(ctx, pvl, gageSclGradVec);
  if (!E) E |= gageUpdate(ctx);
  if (E) {
    fprintf(stderr, "trouble:\n%s\n", biffGetDone(GAGE));
    return 1;
  }

  //TODO: here we might want to run gageContextCopy() multiple times in order to compute gradients properly.

  grad = gageAnswerPointer(ctx, pvl, gageSclGradVec);

  float* gradVec = new float[3*size[0]*size[1]*size[2]];
  float* gradIter = gradVec;
  for(i = 0; i < size[0]; ++i) {
    for(j = 0; j < size[1]; ++j) {
      for(k = 0; k < size[2]; ++k) {
        if( gageProbe(ctx, i, j, k) )
          return 1;//err
        *(gradIter++) = (float)grad[0];
        *(gradIter++) = (float)grad[1];
        *(gradIter++) = (float)grad[2];
      }
    }
  }

  ctx = gageContextNix(ctx); 
  pvl = NULL;
  delete[] gradVec;
*/
  nrrdNuke(nin);
}

FABRIC_EXT_EXPORT void FabricTeemNRRDLoadUShortFromFileHandle(
  const KL::String & handle,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::Size &imageDepth,
  KL::VariableArray<KL::Byte> &imageUShortVoxels,
  KL::Mat44 &xfoMat
  )
{
  KL::FileHandleWrapper wrapper(handle);
  wrapper.ensureIsValidFile();
  return FabricTeemNRRDLoadUShortFromFile(wrapper.getPath(),imageWidth,imageHeight,imageDepth,imageUShortVoxels,xfoMat);
}

FABRIC_EXT_EXPORT void FabricTeemNRRDLoadUShort(
  KL::Data data,
  KL::Size dataSize,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::Size &imageDepth,
  KL::VariableArray<KL::Byte> &imageUShortVoxels,
  KL::Mat44 &xfoMat
  )
{
  imageWidth = 0;
  imageHeight = 0;
  imageDepth = 0;

  imageUShortVoxels.resize( 0 );

  //The library expects a file; so create a temporary one until we have use File-based IO in Fabric
#if defined(FABRIC_OS_WINDOWS)
  char const *dir = getenv("APPDATA");
  if(dir == NULL)
    dir = getenv("TEMP");
  if(dir == NULL)
    dir = getenv("TMP");
  if(dir == NULL)
    Fabric::EDK::throwException("FabricTeemNRRDLoadUShort: environment variable APP_DATA or TMP or TEMP is undefined");
  KL::String fileName( _tempnam( dir, "tmpfab_" ) );
#else
  KL::String fileName(tmpnam(NULL));
#endif

  FILE * file = fopen(fileName.data(),"wb");
  fwrite(data,dataSize,1,file);
  fclose(file);
  file = NULL;

  return FabricTeemNRRDLoadUShortFromFile(fileName,imageWidth,imageHeight,imageDepth,imageUShortVoxels,xfoMat);
}

