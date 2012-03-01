/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>
#include "objparser.h"

#include <streambuf>
#include <fstream>

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

FABRIC_EXT_KL_STRUCT( OBJDataHandle, {
  ObjParser * m_parseData;
} );

struct istreambuf : public std::basic_streambuf<char, std::char_traits<char> >
{
  istreambuf(char* buffer, std::streamsize bufferLength)
  {
    setg(buffer, buffer, buffer + bufferLength);
  }
};

void EDKExceptionThrow(const char* msg) {
  Fabric::EDK::throwException( msg );
}

FABRIC_EXT_EXPORT void FabricOBJFreeParsedData(
  OBJDataHandle& handle
  )
{
  if( handle.m_parseData != NULL )
  {
    delete handle.m_parseData;
    handle.m_parseData = NULL;
  }
}

FABRIC_EXT_EXPORT void FabricOBJDecode(
  KL::Data objData,
  KL::Size objDataSize,
  KL::Boolean splitByObjects,
  KL::Boolean splitByGroups,
  KL::Boolean splitByMaterials,
  OBJDataHandle &handle
  )
{
  istreambuf memstreambuff( (char*)objData,objDataSize );
  std::istream memstream( &memstreambuff );

  FabricOBJFreeParsedData(handle);
  handle.m_parseData = new ObjParser(memstream, splitByObjects, splitByGroups, splitByMaterials, EDKExceptionThrow);
}

FABRIC_EXT_EXPORT void FabricOBJOpenFileHandle(
  const KL::String & fileHandle,
  KL::Boolean splitByObjects,
  KL::Boolean splitByGroups,
  KL::Boolean splitByMaterials,
  OBJDataHandle &handle
  )
{
  KL::FileHandleWrapper wrapper(fileHandle);
  wrapper.ensureIsValidFile();
  std::ifstream filestream(wrapper.getPath().data());
  
  FabricOBJFreeParsedData(handle);
  handle.m_parseData = new ObjParser(filestream, splitByObjects, splitByGroups, splitByMaterials, EDKExceptionThrow);
}

FABRIC_EXT_EXPORT void FabricOBJIsHandleValid(
  const OBJDataHandle& handle,
  KL::Boolean& valid
  )
{
  valid = handle.m_parseData != NULL;
}

FABRIC_EXT_EXPORT void FabricOBJHadErrors(
  const OBJDataHandle& handle,
  KL::Boolean& hadErrors
  )
{
  if( handle.m_parseData == NULL )
    hadErrors = false;
  else
  {
    hadErrors = handle.m_parseData->GetNbErrors() > 0;
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetErrors(
  const OBJDataHandle& handle,
  KL::VariableArray<KL::String>& errors
  )
{
  if( handle.m_parseData == NULL )
    errors.resize( 0 );
  else
  {
    size_t nb = handle.m_parseData->GetNbErrors();
    errors.resize( nb );
    for( size_t i = 0; i < nb; ++i )
      errors[i] = handle.m_parseData->GetError(i).c_str();
  }
}

FABRIC_EXT_EXPORT void FabricOBJHasTextureCoords(
  const OBJDataHandle& handle,
  KL::Boolean& hasTextureCoords
  )
{
  if( handle.m_parseData == NULL )
    hasTextureCoords = false;
  else
    hasTextureCoords = handle.m_parseData->HasTextureCoords();
}

FABRIC_EXT_EXPORT void FabricOBJGetMaterialLibraries(
  const OBJDataHandle& handle,
  KL::VariableArray<KL::String>& materialLibraries
  )
{
  if( handle.m_parseData == NULL )
    materialLibraries.resize(0);
  else
  {
    size_t nb = handle.m_parseData->GetNbMaterialLibraries();
    materialLibraries.resize( nb );
    for( size_t i = 0; i < nb; ++i )
      materialLibraries[i] = handle.m_parseData->GetMaterialLibrary(i).c_str();
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetMaterialNames(
  const OBJDataHandle& handle,
  KL::VariableArray<KL::String>& materialNames
  )
{
  if( handle.m_parseData == NULL )
    materialNames.resize(0);
  else
  {
    size_t nb = handle.m_parseData->GetNbMaterials();
    materialNames.resize( nb );
    for( size_t i = 0; i < nb; ++i )
      materialNames[i] = handle.m_parseData->GetMaterial(i).c_str();
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetNbEntities(
  const OBJDataHandle& handle,
  KL::Size& nbEntities
  )
{
  if( handle.m_parseData == NULL )
    nbEntities = 0;
  else
    nbEntities = handle.m_parseData->GetNbEntities();
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityObjectName(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::String& objectName
  )
{
  if( handle.m_parseData == NULL )
    objectName = "";
  else
    objectName = handle.m_parseData->GetEntityObjectName(entityIndex).c_str();
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityGroupName(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::String& objectName
  )
{
  if( handle.m_parseData == NULL )
    objectName = "";
  else
    objectName = handle.m_parseData->GetEntityGroupName(entityIndex).c_str();
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityMaterialName(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::String& objectName
  )
{
  if( handle.m_parseData == NULL )
    objectName = "";
  else
    objectName = handle.m_parseData->GetEntityMaterialName(entityIndex).c_str();
}

FABRIC_EXT_EXPORT void FabricOBJGetNbEntityPoints(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::Size& nbPoints
  )
{
  if( handle.m_parseData == NULL )
    nbPoints = 0;
  else
    nbPoints = handle.m_parseData->GetNbEntityPoints(entityIndex);
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityPoints(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::VariableArray<KL::Vec3>& points
  )
{
  if( handle.m_parseData == NULL )
    points.resize( 0 );
  else
  {
    size_t nbPoints = handle.m_parseData->GetNbEntityPoints(entityIndex);
    points.resize( nbPoints );

    for( size_t i = 0; i < nbPoints; ++i )
    {
      V3 point = handle.m_parseData->GetEntityPoint( entityIndex, i );
      KL::Vec3 klVec = { point.v[0], point.v[1], point.v[2] };
      points[i] = klVec;
    }
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityPointsSliced(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::SlicedArray<KL::Vec3>& points
  )
{
  if ( handle.m_parseData != NULL )
  {
    size_t nbPoints = handle.m_parseData->GetNbEntityPoints(entityIndex);
    if ( points.size() != nbPoints )
      throwException( "FabricOBJGetEntityPointsSliced: sliced array size mismatch" );

    for( size_t i = 0; i < nbPoints; ++i )
    {
      V3 point = handle.m_parseData->GetEntityPoint( entityIndex, i );
      KL::Vec3 klVec = { point.v[0], point.v[1], point.v[2] };
      points[i] = klVec;
    }
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityNormals(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::VariableArray<KL::Vec3>& normals
  )
{
  if( handle.m_parseData == NULL )
    normals.resize( 0 );
  else
  {
    size_t nbPoints = handle.m_parseData->GetNbEntityPoints(entityIndex);
    normals.resize( nbPoints );

    for( size_t i = 0; i < nbPoints; ++i )
    {
      V3 normal = handle.m_parseData->GetEntityNormal( entityIndex, i );
      KL::Vec3 klVec = { normal.v[0], normal.v[1], normal.v[2] };
      normals[i] = klVec;
    }
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityNormalsSliced(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::SlicedArray<KL::Vec3>& normals
  )
{
  if ( handle.m_parseData )
  {
    size_t nbPoints = handle.m_parseData->GetNbEntityPoints(entityIndex);
    if ( normals.size() != nbPoints )
      throwException( "FabricOBJGetEntityNormalsSliced: sliced array size mismatch" );

    for ( size_t i = 0; i < nbPoints; ++i )
    {
      V3 normal = handle.m_parseData->GetEntityNormal( entityIndex, i );
      KL::Vec3 klVec = { normal.v[0], normal.v[1], normal.v[2] };
      normals[i] = klVec;
    }
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityTextureCoords(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::VariableArray<KL::Vec2>& textureCoords
  )
{
  if( handle.m_parseData == NULL )
    textureCoords.resize( 0 );
  else
  {
    size_t nbPoints = handle.m_parseData->GetNbEntityPoints(entityIndex);
    textureCoords.resize( nbPoints );

    for( size_t i = 0; i < nbPoints; ++i )
    {
      V2 texCoord = handle.m_parseData->GetEntityTextureCoord( entityIndex, i );
      KL::Vec2 klVec = { texCoord.v[0], texCoord.v[1] };
      textureCoords[i] = klVec;
    }
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityTextureCoordsSliced(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::SlicedArray<KL::Vec2>& textureCoords
  )
{
  if ( handle.m_parseData )
  {
    size_t nbPoints = handle.m_parseData->GetNbEntityPoints(entityIndex);
    if ( textureCoords.size() != nbPoints )
      throwException( "FabricOBJGetEntityTextureCoordsSliced: sliced array size mismatch" );

    for ( size_t i = 0; i < nbPoints; ++i )
    {
      V2 texCoord = handle.m_parseData->GetEntityTextureCoord( entityIndex, i );
      KL::Vec2 klVec = { texCoord.v[0], 1.0-texCoord.v[1] };//OBJ considers (0, 0) to be the top left of a texture, OpenGL considers it to be the bottom left
      textureCoords[i] = klVec;
    }
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetNbEntityTriangles(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::Size& nbTriangles
  )
{
  if( handle.m_parseData == NULL )
    nbTriangles = 0;
  else
    nbTriangles = handle.m_parseData->GetNbEntityTriangles(entityIndex);
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityTriangleIndices(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::VariableArray<KL::Integer>& indices
  )
{
  if( handle.m_parseData == NULL )
    indices.resize( 0 );
  else
  {
    size_t nbTriangles = handle.m_parseData->GetNbEntityTriangles(entityIndex);
    indices.resize( nbTriangles * 3 );

    for( size_t i = 0; i < nbTriangles; ++i )
    {
      int t1, t2, t3;
      handle.m_parseData->GetEntityTriangleIndices( entityIndex, i, t1, t2, t3 );
      indices[i*3] = t1;
      indices[i*3+1] = t2;
      indices[i*3+2] = t3;
    }
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityTriangleIndicesSliced(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::SlicedArray<KL::Integer>& indices
  )
{
  if ( handle.m_parseData )
  {
    size_t nbTriangles = handle.m_parseData->GetNbEntityTriangles(entityIndex);
    if ( indices.size() != nbTriangles )
      throwException( "FabricOBJGetEntityTriangleIndicesSliced: sliced array size mismatch" );

    for( size_t i = 0; i < nbTriangles; ++i )
    {
      int t1, t2, t3;
      handle.m_parseData->GetEntityTriangleIndices( entityIndex, i, t1, t2, t3 );
      indices[i*3] = t1;
      indices[i*3+1] = t2;
      indices[i*3+2] = t3;
    }
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityTriangleMaterialIndices(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::VariableArray<KL::Integer>& indices
  )
{
  if( handle.m_parseData == NULL )
    indices.resize( 0 );
  else
  {
    size_t nbTriangles = handle.m_parseData->GetNbEntityTriangles(entityIndex);
    indices.resize( nbTriangles );

    for( size_t i = 0; i < nbTriangles; ++i )
      indices[i] = handle.m_parseData->GetEntityTriangleMaterialIndex( entityIndex, i );
  }
}

FABRIC_EXT_EXPORT void FabricOBJGetEntityTriangleMaterialIndicesSliced(
  const OBJDataHandle& handle,
  KL::Integer entityIndex,
  KL::SlicedArray<KL::Integer>& indices
  )
{
  if ( handle.m_parseData )
  {
    size_t nbTriangles = handle.m_parseData->GetNbEntityTriangles(entityIndex);
    if ( indices.size() != nbTriangles )
      throwException( "FabricOBJGetEntityTriangleIndicesSliced: sliced array size mismatch" );

    for( size_t i = 0; i < nbTriangles; ++i )
      indices[i] = handle.m_parseData->GetEntityTriangleMaterialIndex( entityIndex, i );
  }
}
