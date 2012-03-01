/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef __FABRIC_FILESYSTEM_EXTENSION__
#define __FABRIC_FILESYSTEM_EXTENSION__

#include <Fabric/EDK/EDK.h>
#include <Fabric/Base/RC/Object.h>

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

FABRIC_EXT_KL_STRUCT( FabricFolderHandle, {
  class LocalData : public Fabric::RC::Object {
  public:
    boost::filesystem::path mPath;
  };
  
  LocalData * m_data;
} );

#endif
