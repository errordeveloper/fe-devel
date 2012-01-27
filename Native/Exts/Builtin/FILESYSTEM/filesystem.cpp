/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "filesystem.h"

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

FABRIC_EXT_EXPORT void FabricFolderHandle_Free(
  FabricFolderHandle & folder
)
{
  if(folder.m_data != NULL)
  {
    folder.m_data->release();
    folder.m_data = NULL;
  }
}

FABRIC_EXT_EXPORT void FabricFolderHandle_SetAbsolutePath(
  FabricFolderHandle & folder,
  KL::String & path
)
{
  if(folder.m_data == NULL)
  {
    folder.m_data = new FabricFolderHandle::LocalData();
    folder.m_data->retain();
  }
  folder.m_data->mPath = std::string(path.data());
  if(boost::filesystem::is_regular_file(folder.m_data->mPath))
    folder.m_data->mPath = folder.m_data->mPath.parent_path();
}

FABRIC_EXT_EXPORT void FabricFolderHandle_GetAbsolutePath(
  FabricFolderHandle & folder,
  KL::String& result
)
{
  result = KL::String();
  if(folder.m_data != NULL)
  {
    std::string str = folder.m_data->mPath.string();
    if(!str.empty())
    {
      result.reserve(str.length());
      memcpy((void*)result.data(),str.c_str(),str.length());
    }
  }
}

FABRIC_EXT_EXPORT void FabricFolderHandle_GetBaseName(
  FabricFolderHandle & folder,
  KL::String& result
)
{
  result = KL::String();
  if(folder.m_data != NULL)
  {
    std::string str = folder.m_data->mPath.stem().string();
    if(!str.empty())
    {
      result.reserve(str.length());
      memcpy((void*)result.data(),str.c_str(),str.length());
    }
  }
}

FABRIC_EXT_EXPORT KL::Boolean FabricFolderHandle_IsValid(
  FabricFolderHandle & folder
)
{
  KL::Boolean result = false;
  if(folder.m_data != NULL)
  {
    std::string str = folder.m_data->mPath.string();
    result = !str.empty() && str.length() > 0;
  }
  return result;
}

FABRIC_EXT_EXPORT KL::Boolean FabricFolderHandle_Exists(
  FabricFolderHandle & folder
)
{
  KL::Boolean result = false;
  if(folder.m_data != NULL)
    result = boost::filesystem::exists(folder.m_data->mPath);
  return result;
}

FABRIC_EXT_EXPORT FabricFolderHandle FabricFolderHandle_GetParentFolder(
  const FabricFolderHandle & folder
)
{
  FabricFolderHandle result;
  result.m_data = NULL;
  if(folder.m_data != NULL)
  {
    boost::filesystem::path path = folder.m_data->mPath.parent_path();
    std::string str = path.string();
    if(!str.empty())
    {
      result.m_data = new FabricFolderHandle::LocalData();
      result.m_data->retain();
      result.m_data->mPath = path;
    }
  }
  return result;
}

FABRIC_EXT_EXPORT void FabricFolderHandle_GetSubFolders(
  FabricFolderHandle & folder,
  KL::VariableArray<FabricFolderHandle> & result
)
{
  if(folder.m_data != NULL)
  {
    std::vector<boost::filesystem::path> paths;
    boost::filesystem::directory_iterator it(folder.m_data->mPath);
    boost::filesystem::directory_iterator end;
    while(it != end)
    {
      if(boost::filesystem::is_directory(it->path()))
        paths.push_back(it->path());
      it++;
    }
    result.resize(paths.size());
    for(size_t i=0;i<paths.size();i++)
    {
      result[i].m_data = new FabricFolderHandle::LocalData();
      result[i].m_data->retain();
      result[i].m_data->mPath = paths[i];
    }
  }
}

FABRIC_EXT_EXPORT void FabricFolderHandle_GetFiles(
  FabricFolderHandle & folder,
  KL::VariableArray<KL::FileHandleWrapper> & result
)
{
  if(folder.m_data != NULL)
  {
    std::vector<boost::filesystem::path> paths;
    boost::filesystem::directory_iterator it(folder.m_data->mPath);
    boost::filesystem::directory_iterator end;
    while(it != end)
    {
      if(boost::filesystem::is_regular_file(it->path()))
        paths.push_back(it->path());
      it++;
    }
    result.resize(paths.size());
    for(size_t i=0;i<paths.size();i++)
      result[i].createFromFile(paths[i].string().c_str(),false);
  }
}

FABRIC_EXT_EXPORT void FabricFileHandle_SetAbsolutePath(
  KL::FileHandleWrapper & handle,
  KL::String & path
)
{
  handle.createFromFile(path.data(),false);
}

FABRIC_EXT_EXPORT void FabricFileHandle_GetAbsolutePath(
  KL::FileHandleWrapper & handle,
  KL::String& result
)
{
  result = handle.getPath();
}

FABRIC_EXT_EXPORT FabricFolderHandle FabricFileHandle_GetParentFolder(
  KL::FileHandleWrapper & handle
)
{
  FabricFolderHandle result;
  std::string str( handle.getPath().data() );
  if(!str.empty())
  {
    result.m_data = new FabricFolderHandle::LocalData();
    result.m_data->retain();
    result.m_data->mPath = str;
  }
  return result;
}

FABRIC_EXT_EXPORT void FabricFileHandle_GetName(
  KL::FileHandleWrapper & handle,
  KL::String& result
)
{
  result = KL::String();
  std::string str( handle.getPath().data() );
  if(!str.empty())
  {
    boost::filesystem::path path = str;
    str = path.filename().string();
    result.reserve(str.length());
    memcpy((void*)result.data(),str.c_str(),str.length());
  }
}

FABRIC_EXT_EXPORT void FabricFileHandle_GetBaseName(
  KL::FileHandleWrapper & handle,
  KL::String& result
)
{
  result = KL::String();
  std::string str( handle.getPath().data() );
  if(!str.empty())
  {
    boost::filesystem::path path = str;
    str = path.stem().string();
    result.reserve(str.length());
    memcpy((void*)result.data(),str.c_str(),str.length());
  }
}

FABRIC_EXT_EXPORT void FabricFileHandle_GetExtension(
  KL::FileHandleWrapper & handle,
  KL::String& result
)
{
  result = KL::String();
  std::string str( handle.getPath().data() );
  if(!str.empty())
  {
    boost::filesystem::path path = str;
    str = path.extension().string();
    result.reserve(str.length());
    memcpy((void*)result.data(),str.c_str(),str.length());
  }
}

FABRIC_EXT_EXPORT void FabricFileHandle_GetExtensionLower(
  KL::FileHandleWrapper & handle,
  KL::String& result
)
{
  result = KL::String();
  std::string str( handle.getPath().data() );
  if(!str.empty())
  {
    boost::filesystem::path path = str;
    str = path.extension().string();
    std::transform(str.begin(), str.end(),str.begin(), ::tolower);
    result.reserve(str.length());
    memcpy((void*)result.data(),str.c_str(),str.length());
  }
}

FABRIC_EXT_EXPORT KL::Boolean FabricFileHandle_IsValid(
  const KL::FileHandleWrapper & handle
)
{
  if( !handle.isValid() || handle.isFolder() )
    return false;
  std::string str( handle.getPath().data() );
  return !str.empty();
}

FABRIC_EXT_EXPORT KL::Boolean FabricFileHandle_Exists(
  const KL::FileHandleWrapper & handle
)
{
  KL::Boolean result = false;
  std::string str( handle.getPath().data() );
  if(!str.empty())
  {
    boost::filesystem::path path = str;
    result = boost::filesystem::exists(path);
  }
  return result;
}

FABRIC_EXT_EXPORT KL::Boolean FabricFileHandle_IsReadOnly(
  const KL::FileHandleWrapper & handle
)
{
  return handle.isReadOnly();
}

FABRIC_EXT_EXPORT KL::Size FabricFileHandle_GetSize(
  const KL::FileHandleWrapper & handle
)
{
  KL::Size result = 0;
  std::string str( handle.getPath().data() );
  if(!str.empty())
  {
    boost::filesystem::path path = str;
    result = boost::filesystem::file_size(path);
  }
  return result;
}
