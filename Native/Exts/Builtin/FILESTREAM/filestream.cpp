/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "filestream.h"
#include <stdint.h>
#include <zlib.h>

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

FABRIC_EXT_EXPORT void FabricFileStream_Free(
  FabricFileStream & stream
)
{
  if(stream.m_data != NULL)
  {
    if(stream.m_data->mFile != NULL)
      fclose(stream.m_data->mFile);
    stream.m_data->release();
    stream.m_data = NULL;
  }
}

FABRIC_EXT_EXPORT void FabricFileStream_Open(
  FabricFileStream & stream,
  KL::String & fileHandleString,
  KL::String & mode
)
{
  std::string modeStr = mode.data();
  FabricFileStream_Free(stream);
  KL::FileHandleWrapper file( fileHandleString );
  if( !file.isValid() )
  {
    throwException( "FileHandle '%s' is not a valid fileHandle.", fileHandleString.data() );
    return;
  }
  if( file.isFolder() )
  {
    throwException( "FileHandle '%s' is a folder, invalid for use as a FileStream.", file.getPath().data() );
    return;
  }

  if(modeStr == "r" || modeStr == "w" || modeStr == "a")
  {
    std::string str( file.getPath().data() );
    if(str.empty() || str.length()==0)
      return;
    
    boost::filesystem::path path = str;
    if(modeStr == "r")
    {
      if(!boost::filesystem::exists(path))
      {
        throwException( "FileHandle '%s' does not exist.", file.getPath().data() );
        return;
      }
      if(!boost::filesystem::is_regular_file(path))
      {
        throwException( "FileHandle '%s' is a folder, not valid for use as a FileStream.", file.getPath().data() );
        return;
      }
    }
    else
    {
      if(file.isReadOnly())
      {
        throwException( "FileHandle '%s' is ReadOnly, not valid for use as a writable FileStream.", file.getPath().data() );
        return;
      }
      if(!boost::filesystem::exists(path.parent_path()))
      {
        throwException( "The parent folder of FileHandle '%s' does not exist!", file.getPath().data() );
        return;
      }
      if(!boost::filesystem::is_directory(path.parent_path()))
      {
        throwException( "The parent folder of FileHandle '%s' is not a folder!", file.getPath().data() );
        return;
      }
    }
    
    stream.m_data = new FabricFileStream::LocalData();
    stream.m_data->retain();
    stream.m_data->mMode = modeStr;
    stream.m_data->mReadable = modeStr == "r";
    stream.m_data->mWriteable = !stream.m_data->mReadable;
    stream.m_data->mCloseOnFullyRead = true;
    stream.m_data->mSizeRead = 0;
    if(stream.m_data->mReadable)
    {
      stream.m_data->mSize = boost::filesystem::file_size(path);
      stream.m_data->mSeek = 0;
    }
    else if(modeStr == "w")
    {
      stream.m_data->mSize = 0;
      stream.m_data->mSeek = 0;
    }
    else // "a"
    {
      stream.m_data->mSize = boost::filesystem::file_size(path);
      stream.m_data->mSeek = stream.m_data->mSize;
    }
    stream.m_data->mFile = fopen(path.string().c_str(),modeStr.c_str());
  }
}

FABRIC_EXT_EXPORT void FabricFileStream_CloseOnFullyRead(
  FabricFileStream & stream,
  KL::Boolean & close
)
{
  if(stream.m_data != NULL)
  {
    if(stream.m_data->mReadable)
      stream.m_data->mCloseOnFullyRead = close;
  }
}

FABRIC_EXT_EXPORT KL::Boolean FabricFileStream_IsValid(
  FabricFileStream & stream
)
{
  return stream.m_data != NULL;
}

FABRIC_EXT_EXPORT KL::Boolean FabricFileStream_IsWritable(
  FabricFileStream & stream
)
{
  if(!FabricFileStream_IsValid(stream))
    return false;
  return stream.m_data->mWriteable;
}

FABRIC_EXT_EXPORT KL::Size FabricFileStream_GetSize(
  FabricFileStream & stream
)
{
  if(!FabricFileStream_IsValid(stream))
    return 0;
  return stream.m_data->mSize;
}

FABRIC_EXT_EXPORT KL::Size FabricFileStream_GetSizeRead(
  FabricFileStream & stream
)
{
  if(!FabricFileStream_IsValid(stream))
    return 0;
  return stream.m_data->mSizeRead;
}

FABRIC_EXT_EXPORT KL::Size FabricFileStream_GetSeek(
  FabricFileStream & stream
)
{
  if(!FabricFileStream_IsValid(stream))
    return 0;
  return stream.m_data->mSeek;
}

FABRIC_EXT_EXPORT void FabricFileStream_SetSeek(
  FabricFileStream & stream,
  KL::Size seek
)
{
  if(!FabricFileStream_IsValid(stream))
  {
    throwException( "The FileStream is not valid yet, seeking not possible!" );
    return;
  }
  if(seek == stream.m_data->mSeek)
    return;
  if(seek >= stream.m_data->mSize)
  {
    throwException( "The FileStream cannot seek that far, EOF reached." );
    return;
  }
  fseek(stream.m_data->mFile,(long int)seek,SEEK_SET);
  stream.m_data->mSeek = seek;
}

FABRIC_EXT_EXPORT void FabricFileStream_WriteSize(
  FabricFileStream & stream,
  KL::Size size
)
{
  if(!FabricFileStream_IsValid(stream))
    return;
  if(!stream.m_data->mWriteable)
  {
    throwException( "The FileStream is readOnly, cannot write to it." );
    return;
  }
  uint32_t value = size;
  fwrite(&value,sizeof(uint32_t),1,stream.m_data->mFile);
  stream.m_data->mSeek += sizeof(uint32_t);
  if(stream.m_data->mSeek > stream.m_data->mSize)
    stream.m_data->mSize = stream.m_data->mSeek;
}

FABRIC_EXT_EXPORT void FabricFileStream_ReadSize(
  FabricFileStream & stream,
  KL::Size & size
)
{
  if(!FabricFileStream_IsValid(stream))
    return;
  if(!stream.m_data->mReadable)
  {
    throwException( "The FileStream is writeOnly, cannot read from it." );
    return;
  }
  if(stream.m_data->mSeek + sizeof(uint32_t) > stream.m_data->mSize)
  {
    throwException( "The FileStream does not contain enough data, EOF reached." );
    return;
  }
  uint32_t value = 0;
  size_t readSize = fread(&value,sizeof(uint32_t),1,stream.m_data->mFile);
  size = value;
  stream.m_data->mSizeRead += sizeof(uint32_t);
  stream.m_data->mSeek += sizeof(uint32_t);
  if(stream.m_data->mCloseOnFullyRead && stream.m_data->mSizeRead >= stream.m_data->mSize)
    FabricFileStream_Free(stream);
}

FABRIC_EXT_EXPORT void FabricFileStream_WriteData(
  FabricFileStream & stream,
  KL::Data data,
  KL::Size size
)
{
  if(!FabricFileStream_IsValid(stream))
    return;
  if(!stream.m_data->mWriteable)
  {
    throwException( "The FileStream is readOnly, cannot write to it." );
    return;
  }
  fwrite(data,size,1,stream.m_data->mFile);
  stream.m_data->mSeek += size;
  if(stream.m_data->mSeek > stream.m_data->mSize)
    stream.m_data->mSize = stream.m_data->mSeek;
}

FABRIC_EXT_EXPORT void FabricFileStream_ReadData(
  FabricFileStream & stream,
  KL::Data data,
  KL::Size size
)
{
  if(!FabricFileStream_IsValid(stream))
    return;
  if(!stream.m_data->mReadable)
  {
    throwException( "The FileStream is writeOnly, cannot read from it." );
    return;
  }
  if(stream.m_data->mSeek + size > stream.m_data->mSize)
  {
    throwException( "The FileStream does not contain enough data, EOF reached." );
    return;
  }
  size_t readSize = fread(data,size,1,stream.m_data->mFile);
  stream.m_data->mSizeRead += size;
  stream.m_data->mSeek += size;
  if(stream.m_data->mCloseOnFullyRead && stream.m_data->mSizeRead >= stream.m_data->mSize)
    FabricFileStream_Free(stream);
}

FABRIC_EXT_EXPORT void FabricFileStream_WriteDataCompressed(
  FabricFileStream & stream,
  KL::Data data,
  KL::Size size
)
{
  if(!FabricFileStream_IsValid(stream))
    return;
  if(!stream.m_data->mWriteable)
  {
    throwException( "The FileStream is readOnly, cannot write to it." );
    return;
  }
  
  // compress the data in a temporary buffer
  Bytef * compressed = (Bytef*)malloc(size);
  uLongf compressedSize = size;
  compress2(compressed, &compressedSize, (const Bytef*)data, size, Z_BEST_COMPRESSION);

  // write the original size, as well as the compressed size
  FabricFileStream_WriteSize(stream,size);
  FabricFileStream_WriteSize(stream,compressedSize);
  
  // write the compressed buffer
  fwrite(compressed,compressedSize,1,stream.m_data->mFile);
  stream.m_data->mSeek += compressedSize;
  if(stream.m_data->mSeek > stream.m_data->mSize)
    stream.m_data->mSize = stream.m_data->mSeek;
    
  // free the temporary buffer
  free(compressed);
}

FABRIC_EXT_EXPORT void FabricFileStream_ReadDataCompressed(
  FabricFileStream & stream,
  KL::Data data,
  KL::Size size
)
{
  if(!FabricFileStream_IsValid(stream))
    return;
  if(!stream.m_data->mReadable)
  {
    throwException( "The FileStream is writeOnly, cannot read from it." );
    return;
  }
  
  // write the original size, as well as the compressed size
  KL::Size originalSize = 0;
  KL::Size compressedSize = 0;
  FabricFileStream_ReadSize(stream,originalSize);
  FabricFileStream_ReadSize(stream,compressedSize);

  if(stream.m_data->mSeek + compressedSize > stream.m_data->mSize)
  {
    throwException( "The FileStream does not contain enough data, EOF reached." );
    return;
  }

  // allocate the compressed buffer and fill it
  Bytef * compressed = (Bytef*)malloc(compressedSize);
  size_t readSize = fread(compressed,compressedSize,1,stream.m_data->mFile);
  
  stream.m_data->mSizeRead += compressedSize;
  stream.m_data->mSeek += compressedSize;
  if(stream.m_data->mCloseOnFullyRead && stream.m_data->mSizeRead >= stream.m_data->mSize)
    FabricFileStream_Free(stream);

  // check if we can uncompress
  if(size != originalSize)
  {
    free(compressed);
    throwException( "The original data size doesn't not match the provided one." );
    return;
  }
  
  // perform uncompression
  uLongf uncompressedSize = (uLongf)originalSize;
  uncompress((Bytef*)data, &uncompressedSize, compressed, compressedSize);
  
  // check if the sizes match
  if(uncompressedSize != originalSize)
  {
    free(compressed);
    throwException( "The was an error during uncompression." );
    return;
  }
  
  free(compressed);
}

FABRIC_EXT_EXPORT void FabricFileStream_WriteString(
  FabricFileStream & stream,
  KL::String & string
)
{
  if(!FabricFileStream_IsValid(stream))
    return;
  if(!stream.m_data->mWriteable)
  {
    throwException( "The FileStream is readOnly, cannot write to it." );
    return;
  }
  uint32_t length = string.length();
  fwrite(&length,sizeof(uint32_t),1,stream.m_data->mFile);
  stream.m_data->mSeek += sizeof(uint32_t);
  if(stream.m_data->mSeek > stream.m_data->mSize)
    stream.m_data->mSize = stream.m_data->mSeek;
  if(length > 0)
  {
    fwrite(string.data(),length,1,stream.m_data->mFile);
    stream.m_data->mSeek += length;
    if(stream.m_data->mSeek > stream.m_data->mSize)
      stream.m_data->mSize = stream.m_data->mSeek;
  }
}

FABRIC_EXT_EXPORT void FabricFileStream_ReadString(
  FabricFileStream & stream,
  KL::String & string
)
{
  if(!FabricFileStream_IsValid(stream))
    return;
  if(!stream.m_data->mReadable)
  {
    throwException( "The FileStream is writeOnly, cannot read from it." );
    return;
  }
  if(stream.m_data->mSeek + sizeof(uint32_t) > stream.m_data->mSize)
  {
    throwException( "The FileStream does not contain enough data, EOF reached." );
    return;
  }
  uint32_t length = 0;
  size_t readSize = fread(&length,sizeof(uint32_t),1,stream.m_data->mFile);
  stream.m_data->mSizeRead += sizeof(uint32_t);
  stream.m_data->mSeek += sizeof(uint32_t);

  if(stream.m_data->mSeek + length <= stream.m_data->mSize)
  {
    string.reserve(length);
    if(length > 0)
    {
      readSize = fread((void*)string.data(),length,1,stream.m_data->mFile);
      stream.m_data->mSizeRead += length;
      stream.m_data->mSeek += length;
    }
  }
  if(stream.m_data->mCloseOnFullyRead && stream.m_data->mSizeRead >= stream.m_data->mSize)
    FabricFileStream_Free(stream);
}

FABRIC_EXT_EXPORT void FabricFileStream_WriteStringArray(
  FabricFileStream & stream,
  KL::VariableArray<KL::String> & strings
)
{
  if(!FabricFileStream_IsValid(stream))
    return;
  if(!stream.m_data->mWriteable)
  {
    throwException( "The FileStream is readOnly, cannot write to it." );
    return;
  }
  uint32_t count = strings.size();
  fwrite(&count,sizeof(uint32_t),1,stream.m_data->mFile);
  stream.m_data->mSeek += sizeof(uint32_t);
  if(stream.m_data->mSeek > stream.m_data->mSize)
    stream.m_data->mSize = stream.m_data->mSeek;
  for(uint32_t i=0;i<count;i++)
    FabricFileStream_WriteString(stream,strings[i]);
}

FABRIC_EXT_EXPORT void FabricFileStream_ReadStringArray(
  FabricFileStream & stream,
  KL::VariableArray<KL::String> & strings
)
{
  if(!FabricFileStream_IsValid(stream))
    return;
  if(!stream.m_data->mReadable)
  {
    throwException( "The FileStream is writeOnly, cannot read from it." );
    return;
  }
  if(stream.m_data->mSeek + sizeof(uint32_t) > stream.m_data->mSize)
  {
    throwException( "The FileStream does not contain enough data, EOF reached." );
    return;
  }
  uint32_t count = 0;
  size_t readSize = fread(&count,sizeof(uint32_t),1,stream.m_data->mFile);
  stream.m_data->mSizeRead += sizeof(uint32_t);
  stream.m_data->mSeek += sizeof(uint32_t);
  strings.resize(count);
  for(uint32_t i=0;i<count;i++)
    FabricFileStream_ReadString(stream,strings[i]);
}

