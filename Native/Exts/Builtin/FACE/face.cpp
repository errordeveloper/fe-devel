/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/EDK/EDK.h>

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

#include <opencv/cv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

FABRIC_EXT_KL_STRUCT( FaceLocation, {
  KL::Size left;
  KL::Size right;
  KL::Size top;
  KL::Size bottom;
} );

FABRIC_EXT_KL_STRUCT( FaceDetector, {
  struct LocalData {
    CvHaarClassifierCascade * cascade;
    CvMemStorage * storage;
  };
  LocalData * pointer;
} );

FABRIC_EXT_EXPORT void FabricFACE_DeleteDetector(
  FaceDetector & detector
)
{
  if(detector.pointer)
  {
    cvReleaseHaarClassifierCascade( &detector.pointer->cascade );
    cvReleaseMemStorage( &detector.pointer->storage);
    delete(detector.pointer);
    detector.pointer = NULL;
  }
}

FABRIC_EXT_EXPORT void FabricFACE_CreateDetector(
  FaceDetector & detector,
  const KL::String & fileHandle
)
{
  KL::FileHandleWrapper wrapper(fileHandle);
  wrapper.ensureIsValidFile();

  FabricFACE_DeleteDetector(detector);
  detector.pointer = new FaceDetector::LocalData();
  detector.pointer->cascade = ( CvHaarClassifierCascade* )cvLoad( wrapper.getPath().data(), 0, 0, 0 );
  detector.pointer->storage = cvCreateMemStorage( 0 );
}

FABRIC_EXT_EXPORT void FabricFACE_Detect(
  FaceDetector & detector,
  KL::Data pixels,
  KL::Size width,
  KL::Size height,
  KL::Size channels,
  KL::Scalar ratio,
  KL::Size searchWidth,
  KL::Size searchHeight,
  KL::VariableArray<FaceLocation> & faces
)
{
  if(!detector.pointer)
  {
    Fabric::EDK::throwException("FACE extension: Detector is not initiated.");
    return;
  }
  
  // convert our image to an opencv image
  int mode = CV_8UC1;
  if(channels == 2)
    mode = CV_8UC2;
  else if(channels == 3)
    mode = CV_8UC3;
  else if(channels == 4)
    mode = CV_8UC4;
  else
  {
    Fabric::EDK::throwException("FACE extension: Max 4 channels for images supported.");
    return;
  }
  cv::Mat img(height,width,mode,pixels);
  IplImage imgHeader = img;
  
  /* detect faces */
  CvSeq * results = cvHaarDetectObjects(
    &imgHeader,
    detector.pointer->cascade,
    detector.pointer->storage,
    ratio,
    3,
    0 /*CV_HAAR_DO_CANNY_PRUNNING*/,
    cvSize( searchWidth, searchHeight )
  );
  
  faces.resize(results ? results->total : 0);
  for( int i = 0 ; i < ( results ? results->total : 0 ) ; i++ ) {
    CvRect *r = ( CvRect* )cvGetSeqElem( results, i );
    faces[i].left = r->x;
    faces[i].bottom = r->y;
    faces[i].right = r->x + r->width;
    faces[i].top = r->y + r->height;
  }  

  return;
}
