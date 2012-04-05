/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>
#include <Fabric/Base/RC/Object.h>

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES
//#define LIDAR_TRACE

FABRIC_EXT_KL_STRUCT( Vec3, {
  KL::Float32 x;
  KL::Float32 y;
  KL::Float32 z;
} );

FABRIC_EXT_KL_STRUCT( Color, {
  KL::Float32 r;
  KL::Float32 g;
  KL::Float32 b;
  KL::Float32 a;
} );

#include <string>
#include <iostream>
#include <fstream>
#include <liblas/liblas.hpp>

// ====================================================================
// KL structs
FABRIC_EXT_KL_STRUCT( LidarReader, {
  class LocalData : public Fabric::RC::Object {
  public:
  
    LocalData(KL::String & fileName, KL::Boolean & compressed);
    
    void reset();

    std::ifstream mStream;    
    liblas::Reader * mReader;
  
  protected:
  
    virtual ~LocalData();
  };

  LocalData * localData;
  KL::String url;
  KL::Boolean compressed;
} );

LidarReader::LocalData::LocalData(KL::String & fileName, KL::Boolean & compressed) {
#ifdef LIDAR_TRACE
    log("  { FabricLIDAR } : Opening file %s",fileName.data());
#endif
  mStream.open(fileName.data(), std::ifstream::in | std::ifstream::binary);
  liblas::ReaderFactory f;
  mReader = NULL;
  mReader = new liblas::Reader(f.CreateWithStream(mStream));
}

LidarReader::LocalData::~LocalData() {
#ifdef LIDAR_TRACE
    log("  { FabricLIDAR } : Calling LidarReader::LocalData::~LocalData()");
#endif
  if(mReader != NULL)
    delete(mReader);
  mStream.clear();
}

void FabricLIDAR_Reader_Open(
  KL::String & fileName,
  LidarReader & lidar
)
{
  if(lidar.localData == NULL && fileName.data() != NULL) {
#ifdef LIDAR_TRACE
    log("  { FabricLIDAR} : FabricLIDAR_Reader_Open called.");
#endif

    lidar.compressed = lidar.url.data()[lidar.url.length()-1] == 'z' || lidar.url.data()[lidar.url.length()-1] == 'Z';
    try
    {
      lidar.localData = new LidarReader::LocalData(fileName,lidar.compressed);
    }
    catch(std::runtime_error e)
    {
      printf("  { FabricLIDAR } : RuntimeError caught: '%s'.\n",e.what());
      Fabric::EDK::throwException("  { FabricLIDAR } : RuntimeError caught: '%s'",e.what());
      lidar.localData = NULL;
    }
    catch(std::exception e)
    {
      printf("  { FabricLIDAR } : Exception caught: '%s'.\n",e.what());
      Fabric::EDK::throwException("  { FabricLIDAR } : Exception caught: '%s'",e.what());
      lidar.localData = NULL;
    }

#ifdef LIDAR_TRACE
    log("  { FabricLIDAR } : FabricLIDAR_Reader_Open completed.");
#endif
  }
}

FABRIC_EXT_EXPORT void FabricLIDAR_Reader_Decode(
  KL::Data resourceData,
  KL::Size resourceDataSize,
  LidarReader & lidar
)
{
  if(lidar.localData == NULL && resourceData != NULL && resourceDataSize > 0) {
#ifdef LIDAR_TRACE
    log("  { FabricLIDAR} : FabricLIDAR_Reader_Decode called.");
#endif

#if defined(FABRIC_OS_WINDOWS)
    char const *dir = getenv("TEMP");
    if(dir == NULL)
      dir = getenv("TMP");
    if(dir == NULL)
      dir = getenv("APPDATA");
    if(dir == NULL)
      Fabric::EDK::throwException("  { FabricLIDAR } : environment variable APP_DATA or TMP or TEMP is undefined");
    KL::String fileName( _tempnam( dir, "tmpfab_" ) );
#else
    KL::String fileName(tmpnam(NULL));
#endif

    // save the file to disk
    FILE * file = fopen(fileName.data(),"wb");
    if(!file)
      Fabric::EDK::throwException("  { FabricLIDAR } : Cannot write to temporary file.");
    fwrite(resourceData,resourceDataSize,1,file);
    fclose(file);
    file = NULL;
    
    FabricLIDAR_Reader_Open(fileName,lidar);

#ifdef LIDAR_TRACE
    log("  { FabricLIDAR } : FabricLIDAR_Reader_Decode completed.");
#endif
  }
}

FABRIC_EXT_EXPORT void FabricLIDAR_Reader_OpenFileHandle(
  const KL::String & handle,
  LidarReader & lidar
)
{
  if(lidar.localData == NULL) {
#ifdef LIDAR_TRACE
    log("  { FabricLIDAR} : FabricLIDAR_Reader_OpenFileHandle called.");
#endif

    KL::FileHandleWrapper wrapper(handle);
    wrapper.ensureIsValidFile();
    KL::String fileName = wrapper.getPath();
    FabricLIDAR_Reader_Open(fileName,lidar);

#ifdef LIDAR_TRACE
    log("  { FabricLIDAR } : FabricLIDAR_Reader_OpenFileHandle completed.");
#endif
  }
}

FABRIC_EXT_EXPORT void FabricLIDAR_Reader_Free(
  LidarReader & lidar
)
{
  if(lidar.localData != NULL) {
#ifdef LIDAR_TRACE
    log("  { FabricLIDAR} : FabricLIDAR_Reader_Free called.");
#endif

    lidar.localData->release();
    lidar.localData = NULL;

#ifdef LIDAR_TRACE
    log("  { FabricLIDAR } : FabricLIDAR_Reader_Free completed.");
#endif
  }
}

FABRIC_EXT_EXPORT void FabricLIDAR_Reader_GetCount(
  LidarReader & lidar,
  KL::Size & count
)
{
  if(lidar.localData != NULL) {
#ifdef LIDAR_TRACE
    log("  { FabricLIDAR} : FabricLIDAR_Reader_Count called.");
#endif

    try
    {
      count = (KL::Size)lidar.localData->mReader->GetHeader().GetPointRecordsCount();
    }
    catch(std::runtime_error e)
    {
      printf("  { FabricLIDAR } : RuntimeError caught: '%s'.\n",e.what());
      Fabric::EDK::throwException("  { FabricLIDAR } : RuntimeError caught: '%s'",e.what());
    }
    catch(std::exception e)
    {
      printf("  { FabricLIDAR } : Exception caught: '%s'.\n",e.what());
      Fabric::EDK::throwException("  { FabricLIDAR } : Exception caught: '%s'",e.what());
    }

#ifdef LIDAR_TRACE
    log("  { FabricLIDAR } : FabricLIDAR_Reader_Count completed.");
#endif
  }
}

FABRIC_EXT_EXPORT void FabricLIDAR_Reader_GetPoints(
  LidarReader & lidar,
  KL::SlicedArray<Vec3>& positions,
  KL::SlicedArray<Color>& colors
)
{
  if(lidar.localData != NULL) {
#ifdef LIDAR_TRACE
    log("  { FabricLIDAR} : FabricLIDAR_Reader_GetPoints called.");
#endif

    // only do this if the counts match
    KL::Size count = 0;
    try
    {
      count = (KL::Size)lidar.localData->mReader->GetHeader().GetPointRecordsCount();
    }
    catch(std::runtime_error e)
    {
      printf("  { FabricLIDAR } : RuntimeError caught: '%s'.\n",e.what());
      Fabric::EDK::throwException("  { FabricLIDAR } : RuntimeError caught: '%s'",e.what());
    }
    catch(std::exception e)
    {
      printf("  { FabricLIDAR } : Exception caught: '%s'.\n",e.what());
      Fabric::EDK::throwException("  { FabricLIDAR } : Exception caught: '%s'",e.what());
    }
    if(positions.size() == count)
    {
      KL::Size offset = 0;
      try
      {
        while (lidar.localData->mReader->ReadNextPoint())
        {
            liblas::Point const& p = lidar.localData->mReader->GetPoint();
            positions[offset].x = (KL::Scalar)p.GetX();
            positions[offset].y = (KL::Scalar)p.GetZ();
            positions[offset].z = (KL::Scalar)p.GetY();
            colors[offset].r = float(p.GetColor().GetRed()) / 255.0f;
            colors[offset].g = float(p.GetColor().GetGreen()) / 255.0f;
            colors[offset].b = float(p.GetColor().GetBlue()) / 255.0f;
            colors[offset].a = 1.0;
            offset++;
        }
      }
      catch(std::runtime_error e)
      {
        printf("  { FabricLIDAR } : RuntimeError caught: '%s'.\n",e.what());
        Fabric::EDK::throwException("  { FabricLIDAR } : RuntimeError caught: '%s'",e.what());
      }
      catch(std::exception e)
      {
        printf("  { FabricLIDAR } : Exception caught: '%s'.\n",e.what());
        Fabric::EDK::throwException("  { FabricLIDAR } : Exception caught: '%s'",e.what());
      }
    }

#ifdef LIDAR_TRACE
    log("  { FabricLIDAR } : FabricLIDAR_Reader_GetPoints completed.");
#endif
  }
}
