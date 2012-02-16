/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/EDK/EDK.h>

#define cimg_display 0
#define cimg_use_tif 1
#define cimg_use_tiff 1
#define cimg_use_jpeg 1

#include "CImg.h"

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

using namespace cimg_library;

void readImageFromFile(
  const KL::String & fileName,
  const KL::String & ext,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::VariableArray<KL::RGBA> &imagePixels
  )
{
  CImg<unsigned char> img;
  try{
    img.load(fileName.data(), ext.data());
  }catch(CImgIOException e) {
    throwException("CIMG Exception: %s",e.what());
    return;
  }
  unsigned char * imgData = img.data();
  imageWidth = img.width();
  imageHeight = img.height();
  
  size_t nbPixels = imageWidth * imageHeight;
  size_t offsetG = nbPixels;
  size_t offsetB = nbPixels * 2;
  size_t offsetA = nbPixels * 3;
  
  imagePixels.resize(nbPixels);
  if(img.spectrum() == 1)
  {
    for(size_t i=0;i<nbPixels;i++)
    {
      imagePixels[i].r = imgData[i];
      imagePixels[i].g = imgData[i];
      imagePixels[i].b = imgData[i];
      imagePixels[i].a = 255;
    }
  }
  else if(img.spectrum() == 3)
  {
    for(size_t i=0;i<nbPixels;i++,offsetG++,offsetB++)
    {
      imagePixels[i].r = imgData[i];
      imagePixels[i].g = imgData[offsetG];
      imagePixels[i].b = imgData[offsetB];
      imagePixels[i].a = 255;
    }
  }
  else if(img.spectrum() == 4)
  {
    for(size_t i=0;i<nbPixels;i++,offsetG++,offsetB++,offsetA++)
    {
      imagePixels[i].r = imgData[i];
      imagePixels[i].g = imgData[offsetG];
      imagePixels[i].b = imgData[offsetB];
      imagePixels[i].a = imgData[offsetA];
    }
  }
}


FABRIC_EXT_EXPORT void FabricCIMGDecode(
  KL::Data data,
  const KL::String & ext,
  KL::Size dataSize,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::VariableArray<KL::RGBA> &imagePixels
  )
{
#if defined(FABRIC_OS_WINDOWS)
  char const *dir = getenv("APPDATA");
  if(dir == NULL)
    dir = getenv("TEMP");
  if(dir == NULL)
    dir = getenv("TMP");
  if(dir == NULL)
    Fabric::EDK::throwException("CIMG extension: environment variable APP_DATA or TMP or TEMP is undefined");
  KL::String fileName( _tempnam( dir, "tmpfab_" ) );
#else
  KL::String fileName(tmpnam(NULL));
#endif

  // save the file to disk
  FILE * file = fopen(fileName.data(),"wb");
  if(!file)
    Fabric::EDK::throwException("CIMG extension: Cannot write to temporary file.");
  fwrite(data,dataSize,1,file);
  fclose(file);
  file = NULL;
  
  return readImageFromFile(fileName,ext,imageWidth,imageHeight,imagePixels);
}

FABRIC_EXT_EXPORT void FabricCIMGOpenFileHandle(
  const KL::String & fileHandle,
  const KL::String & ext,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::VariableArray<KL::RGBA> &imagePixels
  )
{
  KL::FileHandleWrapper wrapper(fileHandle);
  wrapper.ensureIsValidFile();
  return readImageFromFile(wrapper.getPath(),ext,imageWidth,imageHeight,imagePixels);
}

FABRIC_EXT_EXPORT void FabricCIMGCreateFromText(
  const KL::String & text,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::VariableArray<KL::RGBA> &imagePixels
  )
{
  unsigned char foreground = 255;
  unsigned char background = 0;
  CImg<unsigned char> image;
  image.draw_text(
    0,
    0,
    text.data(),
    &foreground,
    &background,
    1.0f,
    24
  );
 
  // resize the image
  imageWidth = (KL::Size)image.width();
  imageHeight = (KL::Size)image.height();
  imagePixels.resize(imageWidth*imageHeight);
 
  // copy the image over
  for(KL::Size i=0;i<imagePixels.size();i++)
    imagePixels[i].r = imagePixels[i].g = imagePixels[i].b = imagePixels[i].a = image.data()[i];
}

FABRIC_EXT_EXPORT void FabricCIMGSaveToFileHandle(
  const KL::String & fileHandle,
  KL::Size imageWidth,
  KL::Size imageHeight,
  KL::Boolean mirrorVertically,
  KL::VariableArray<KL::RGBA> &imagePixels
)
{
  KL::FileHandleWrapper wrapper(fileHandle);
  if(wrapper.isFolder())
  {
    Fabric::EDK::throwException("CIMG extension: Cannot write to a folder.");
    return;
  }
  if(wrapper.isReadOnly())
  {
    Fabric::EDK::throwException("CIMG extension: Cannot write to a readOnly FileHandle.");
    return;
  }
  wrapper.ensureTargetExists();
  
  CImg<unsigned char> image(imageWidth,imageHeight,1,4);
  KL::Size offsetR = 0;
  KL::Size offsetG = offsetR + imagePixels.size();
  KL::Size offsetB = offsetG + imagePixels.size();
  KL::Size offsetA = offsetB + imagePixels.size();
  
  for(KL::Size i=0;i<imagePixels.size();i++)
  {
    image.data()[offsetR++] = imagePixels[i].r;
    image.data()[offsetG++] = imagePixels[i].g;
    image.data()[offsetB++] = imagePixels[i].b;
    image.data()[offsetA++] = imagePixels[i].a;
  }
  
  if(mirrorVertically)
    image.mirror('y');
  
  try{
    image.save(wrapper.getPath().data());
  }catch(CImgIOException e) {
    throwException("CIMG Exception: %s",e.what());
    return;
  }
}