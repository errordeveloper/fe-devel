/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef __FABRIC_FILESTREAM_EXTENSION__
#define __FABRIC_FILESTREAM_EXTENSION__

#include <Fabric/EDK/EDK.h>
#include <Fabric/Base/RC/Object.h>

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

FABRIC_EXT_KL_STRUCT( FabricFileStream, {
  class LocalData : public Fabric::RC::Object {
  public:
    std::string mMode;
    bool mReadable;
    bool mWriteable;
    bool mCloseOnFullyRead;
    size_t mSeek;
    size_t mSize;
    size_t mSizeRead;
    FILE * mFile;
  };
  
  LocalData * m_data;
} );

#endif
