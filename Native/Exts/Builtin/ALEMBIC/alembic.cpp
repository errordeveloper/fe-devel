/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>

//#include <boost/smart_ptr.hpp>
//#include <boost/format.hpp>
//#include <boost/variant.hpp>
#include <boost/algorithm/string.hpp>

#include <utility>
#include <limits>

#include <set>
#include <vector>
#include <map>
#include <list>

#include <stdexcept>
#include <exception>

#include <string>

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/Abc/All.h>

namespace AbcA = ::Alembic::AbcCoreAbstract::ALEMBIC_VERSION_NS;
using namespace AbcA;

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

struct AlembicHandle{
  KL::Data pointer;
  KL::Vec2 timeRange;
};

struct SampleInfo
{
   Alembic::AbcCoreAbstract::index_t floorIndex;
   Alembic::AbcCoreAbstract::index_t ceilIndex;
   double alpha;
};

SampleInfo getSampleInfo
(
   double iFrame,
   Alembic::AbcCoreAbstract::TimeSamplingPtr iTime,
   size_t numSamps
)
{
   SampleInfo result;
   if (numSamps == 0)
      numSamps = 1;

   std::pair<Alembic::AbcCoreAbstract::index_t, double> floorIndex =
   iTime->getFloorIndex(iFrame, numSamps);

   result.floorIndex = floorIndex.first;
   result.ceilIndex = result.floorIndex;

   if (fabs(iFrame - floorIndex.second) < 0.0001) {
      result.alpha = 0.0f;
      return result;
   }

   std::pair<Alembic::AbcCoreAbstract::index_t, double> ceilIndex =
   iTime->getCeilIndex(iFrame, numSamps);

   if (fabs(iFrame - ceilIndex.second) < 0.0001) {
      result.floorIndex = ceilIndex.first;
      result.ceilIndex = result.floorIndex;
      result.alpha = 0.0f;
      return result;
   }

   if (result.floorIndex == ceilIndex.first) {
      result.alpha = 0.0f;
      return result;
   }

   result.ceilIndex = ceilIndex.first;

   result.alpha = (iFrame - floorIndex.second) / (ceilIndex.second - floorIndex.second);
   return result;
}

Alembic::Abc::IObject getObjectFromArchive(Alembic::Abc::IArchive * archive, const std::string & identifier)
{
  // split the path
  std::vector<std::string> parts;
  boost::split(parts, identifier, boost::is_any_of("/"));

  // recurse to find it
  Alembic::Abc::IObject obj = archive->getTop();
  for(size_t i=1;i<parts.size();i++)
  {
    Alembic::Abc::IObject child(obj,parts[i]);
    obj = child;
  }
    
  return obj; 
}

FABRIC_EXT_EXPORT void FabricALEMBICOpen(
  const KL::String& fileName,
  AlembicHandle &handle
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if( archive == NULL )
  {
    // load the file    
    archive = new Alembic::Abc::IArchive(
      Alembic::AbcCoreHDF5::ReadArchive(),fileName.data());
    handle.pointer = archive;
    
    // determine the time range
    handle.timeRange.x = 1000000;
    handle.timeRange.y = -1000000;
    
    // let's skip the first one
    for(uint32_t i=1;i<archive->getNumTimeSamplings();i++){
      AbcA::TimeSamplingPtr ts = archive->getTimeSampling(i);
      const std::vector<chrono_t> & times = ts->getStoredTimes();
      if(times.size() == 0)
        continue;
      if(handle.timeRange.x > times[0])
        handle.timeRange.x = times[0];
      if(handle.timeRange.y < times[times.size()-1])
        handle.timeRange.y = times[times.size()-1];
    }
    
    if(handle.timeRange.x == 1000000) {
      handle.timeRange.x = handle.timeRange.y = 0.0f;
    }
  }
}


FABRIC_EXT_EXPORT void FabricALEMBICOpenFileHandle(
  const KL::String& fileHandle,
  AlembicHandle &handle
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if( archive == NULL )
  {
    KL::FileHandleWrapper fileWrapper( fileHandle );
    fileWrapper.ensureIsValidFile();
    return FabricALEMBICOpen(fileWrapper.getPath(),handle);
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICFreeAlembicHandle(
  AlembicHandle &handle
)
{
  if(handle.pointer != NULL) {
    Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
    delete(archive);
    handle.pointer = NULL;
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICGetIdentifiers(
  AlembicHandle &handle,
  KL::VariableArray<KL::String> & identifiers
)
{
  identifiers.resize(0);
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if( archive != NULL )
  {
    std::vector<Alembic::Abc::IObject> iObjects;
    iObjects.push_back(archive->getTop());

    size_t offset = 0;
    for(size_t i=0;i<iObjects.size();i++)
    {
      Alembic::Abc::IObject obj(iObjects[i],Alembic::Abc::kWrapExisting);
      
      for(size_t j=0;j<obj.getNumChildren();j++)
      {
        Alembic::Abc::IObject child = obj.getChild(j);

        const Alembic::Abc::MetaData &md = child.getMetaData();
        if(Alembic::AbcGeom::IXformSchema::matches(md) ||
           Alembic::AbcGeom::IPolyMesh::matches(md)||
           Alembic::AbcGeom::ICurves::matches(md)||
           Alembic::AbcGeom::INuPatch::matches(md)||
           Alembic::AbcGeom::IPoints::matches(md)||
           Alembic::AbcGeom::ISubD::matches(md)||
           Alembic::AbcGeom::ICamera::matches(md))
        {
          iObjects.push_back(child);
          offset++;
        }
      }
    }
    
    identifiers.resize(offset);
    offset = 0;
    for(size_t i=1;i<iObjects.size();i++)
    {
      std::string identifier = iObjects[i].getFullName().c_str();
      identifier += "|";
      int numSamples = 1;
      const Alembic::Abc::MetaData &md = iObjects[i].getMetaData();
      if(Alembic::AbcGeom::IXform::matches(md)) {
        identifier += "Xform|";
        numSamples = Alembic::AbcGeom::IXform(iObjects[i],Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
      } else if(Alembic::AbcGeom::IPolyMesh::matches(md)) {
        identifier += "PolyMesh|";
        numSamples = Alembic::AbcGeom::IPolyMesh(iObjects[i],Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
      } else if(Alembic::AbcGeom::ICurves::matches(md)) {
        identifier += "Curves|";
        numSamples = Alembic::AbcGeom::ICurves(iObjects[i],Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
      } else if(Alembic::AbcGeom::INuPatch::matches(md)) {
        identifier += "NuPatch|";
        numSamples = Alembic::AbcGeom::INuPatch(iObjects[i],Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
      } else if(Alembic::AbcGeom::IPoints::matches(md)) {
        identifier += "Points|";
        numSamples = Alembic::AbcGeom::IPoints(iObjects[i],Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
      } else if(Alembic::AbcGeom::ISubD::matches(md)) {
        identifier += "SubD|";
        numSamples = Alembic::AbcGeom::ISubD(iObjects[i],Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
      } else if(Alembic::AbcGeom::ICamera::matches(md)) {
        identifier += "Camera|";
        numSamples = Alembic::AbcGeom::ICamera(iObjects[i],Alembic::Abc::kWrapExisting).getSchema().getNumSamples();
      }
      identifier += boost::lexical_cast<std::string>(numSamples).c_str();
      identifiers[offset] = identifier.c_str();
      offset++;
    }
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICParseXform(
  AlembicHandle &handle,
  KL::String & identifier,
  KL::Scalar & time,
  KL::Xfo & transform
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if(archive == NULL)
    return;
    
  Alembic::AbcGeom::IXform obj(getObjectFromArchive(archive,identifier.data()),Alembic::Abc::kWrapExisting);
  if( obj.valid() )
  {
    // get the schema
    Alembic::AbcGeom::IXformSchema schema = obj.getSchema();

    // get the sample information
    SampleInfo sampleInfo = getSampleInfo(time,schema.getTimeSampling(),schema.getNumSamples());
      
    // get the sample
    Alembic::AbcGeom::XformSample sample;
    schema.get(sample,sampleInfo.floorIndex);

    // blend
    Alembic::Abc::M44d matrix = sample.getMatrix();
    if(sampleInfo.alpha != 0.0)
    {
      obj.getSchema().get(sample,sampleInfo.ceilIndex);
      Alembic::Abc::M44d ceilMatrix = sample.getMatrix();
      matrix = (1.0 - sampleInfo.alpha) * matrix + sampleInfo.alpha * ceilMatrix;
    }
    
    Alembic::Abc::Quatd ori = extractQuat(matrix);
    Alembic::Abc::V3d sc;
    extractScaling(matrix,sc);
    
    transform.ori.v.x = (KL::Scalar)ori.v.x;
    transform.ori.v.y = (KL::Scalar)ori.v.y;
    transform.ori.v.z = (KL::Scalar)ori.v.z;
    transform.ori.w = (KL::Scalar)ori.r;
    transform.tr.x = (KL::Scalar)matrix[3][0];
    transform.tr.y = (KL::Scalar)matrix[3][1];
    transform.tr.z = (KL::Scalar)matrix[3][2];
    transform.sc.x = (KL::Scalar)sc.x;
    transform.sc.y = (KL::Scalar)sc.y;
    transform.sc.z = (KL::Scalar)sc.z;
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICParseCamera(
  AlembicHandle &handle,
  KL::String & identifier,
  KL::Scalar & time,
  KL::Scalar & near,
  KL::Scalar & far,
  KL::Scalar & fovY
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if(archive == NULL)
    return;
    
  Alembic::AbcGeom::ICamera obj(getObjectFromArchive(archive,identifier.data()),Alembic::Abc::kWrapExisting);
  if( obj.valid() )
  {
    // get the schema
    Alembic::AbcGeom::ICameraSchema schema = obj.getSchema();
      
    // get the sample information
    SampleInfo sampleInfo = getSampleInfo(time,schema.getTimeSampling(),schema.getNumSamples());

    // get the sample
    Alembic::AbcGeom::CameraSample sample;
    schema.get(sample,sampleInfo.floorIndex);

    // clipping planes
    near = sample.getNearClippingPlane();    
    far = sample.getFarClippingPlane();
    
    // compute fovY from aspect and aperture
    KL::Scalar focallength = sample.getFocalLength();
    KL::Scalar aperture = sample.getVerticalAperture();
    fovY = 1.0f * atanf(50.0f * aperture / focallength);
    
    // blend
    if(sampleInfo.alpha != 0.0)
    {
      schema.get(sample,sampleInfo.ceilIndex);
      float ialpha = 1.0f - sampleInfo.alpha;
      near = ialpha * near + sampleInfo.alpha * sample.getNearClippingPlane();
      far = ialpha * far + sampleInfo.alpha * sample.getFarClippingPlane();
      focallength = sample.getFocalLength();
      aperture = sample.getVerticalAperture();
      fovY = ialpha * fovY + sampleInfo.alpha * (1.0f * atanf(50.0f * aperture / focallength));
    }
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICParsePolyMeshUniforms(
  AlembicHandle &handle,
  KL::String & identifier,
  KL::VariableArray<KL::Integer>& indices
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if(archive == NULL)
    return;
    
  Alembic::AbcGeom::IPolyMesh obj(getObjectFromArchive(archive,identifier.data()),Alembic::Abc::kWrapExisting);
  if( obj.valid() )
  {
    // get the schema
    Alembic::AbcGeom::IPolyMeshSchema schema = obj.getSchema();

    // get the sample
    Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
    schema.get(sample,0);

    // store the indices
    std::vector<KL::Integer> vecIndices;
    Alembic::Abc::Int32ArraySamplePtr faceCounts = sample.getFaceCounts();
    for(size_t i=0;i<faceCounts->size();i++)
    {
      size_t curSize = vecIndices.size();
      if(faceCounts->get()[i] == 3)
      {
        vecIndices.push_back(curSize + 2);
        vecIndices.push_back(curSize + 1);
        vecIndices.push_back(curSize);
      }
      else if(faceCounts->get()[i] == 4)
      {
        vecIndices.push_back(curSize + 2);
        vecIndices.push_back(curSize + 1);
        vecIndices.push_back(curSize);
        vecIndices.push_back(curSize + 5);
        vecIndices.push_back(curSize + 4);
        vecIndices.push_back(curSize + 3);
      }
    }
    
    // finally copy them out
    indices.resize(vecIndices.size());
    memcpy(&indices[0],&vecIndices[0],sizeof(KL::Integer) * vecIndices.size());
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICParsePolyMeshCount(
  AlembicHandle &handle,
  KL::String & identifier,
  KL::Size & count
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if(archive == NULL)
    return;
    
  Alembic::AbcGeom::IPolyMesh obj(getObjectFromArchive( archive,identifier.data()),Alembic::Abc::kWrapExisting);
  if( obj.valid() )
  {
    // get the schema
    Alembic::AbcGeom::IPolyMeshSchema schema = obj.getSchema();
      
    // get the sample
    Alembic::AbcGeom::IPolyMeshSchema::Sample sample;
    schema.get(sample,0);

    // loop on all counts
    Alembic::Abc::Int32ArraySamplePtr faceCounts = sample.getFaceCounts();
    count = 0;
    for(size_t i=0;i<faceCounts->size();i++)
    {
      if(faceCounts->get()[i] == 3)
        count += 3;
      else if(faceCounts->get()[i] == 4)
        count += 6;
    }
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICParsePolyMeshAttributes(
  AlembicHandle &handle,
  KL::String & identifier,
  KL::Scalar & time,
  KL::SlicedArray<KL::Vec3>& vertices,
  KL::SlicedArray<KL::Vec3>& normals,
  KL::Boolean & uvsLoaded,
  KL::SlicedArray<KL::Vec2>& uvs
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if(archive == NULL)
    return;
    
  Alembic::AbcGeom::IPolyMesh obj(getObjectFromArchive(archive,identifier.data()),Alembic::Abc::kWrapExisting);
  if( obj.valid() )
  {
    // get the schema
    Alembic::AbcGeom::IPolyMeshSchema schema = obj.getSchema();

    // get the sample information
    SampleInfo sampleInfo = getSampleInfo(time,schema.getTimeSampling(),schema.getNumSamples());
    
    // get the sample
    Alembic::AbcGeom::IPolyMeshSchema::Sample sample = schema.getValue(sampleInfo.floorIndex);
    Alembic::AbcGeom::IPolyMeshSchema::Sample sampleB;
    Alembic::Abc::P3fArraySamplePtr abcPoints = sample.getPositions();
    Alembic::Abc::P3fArraySamplePtr abcPointsB;
    bool requiresInterpolation = sampleInfo.alpha != 0.0;
    if(requiresInterpolation)
    {
       sampleB = schema.getValue(sampleInfo.ceilIndex);
       abcPointsB = sampleB.getPositions();
    }
    float iAlpha = 1.0 - sampleInfo.alpha;
    
    Alembic::Abc::Int32ArraySamplePtr faceCounts = sample.getFaceCounts();
    Alembic::Abc::Int32ArraySamplePtr faceIndices= sample.getFaceIndices();
    
    // load the vertices
    size_t vertexOffset = 0;
    size_t indexOffset = 0;
    size_t currentIndex;
    for(size_t i=0;i<faceCounts->size();i++)
    {
      size_t count = faceCounts->get()[i];
      if(count == 3)
      {
        for(int j=0;j<3;j++)
        {
          currentIndex = indexOffset + j;
          vertices[vertexOffset].x = iAlpha * abcPoints->get()[faceIndices->get()[currentIndex]].x;
          vertices[vertexOffset].y = iAlpha * abcPoints->get()[faceIndices->get()[currentIndex]].y;
          vertices[vertexOffset].z = iAlpha * abcPoints->get()[faceIndices->get()[currentIndex]].z;
          if(requiresInterpolation)
          {
            vertices[vertexOffset].x += sampleInfo.alpha * abcPointsB->get()[faceIndices->get()[currentIndex]].x;
            vertices[vertexOffset].y += sampleInfo.alpha * abcPointsB->get()[faceIndices->get()[currentIndex]].y;
            vertices[vertexOffset].z += sampleInfo.alpha * abcPointsB->get()[faceIndices->get()[currentIndex]].z;
          }
          vertexOffset++;
        }
      }
      else if(count == 4)
      {
        for(int j=0;j<3;j++)
        {
          currentIndex = indexOffset + j;
          vertices[vertexOffset].x = iAlpha * abcPoints->get()[faceIndices->get()[currentIndex]].x;
          vertices[vertexOffset].y = iAlpha * abcPoints->get()[faceIndices->get()[currentIndex]].y;
          vertices[vertexOffset].z = iAlpha * abcPoints->get()[faceIndices->get()[currentIndex]].z;
          if(requiresInterpolation)
          {
            vertices[vertexOffset].x += sampleInfo.alpha * abcPointsB->get()[faceIndices->get()[currentIndex]].x;
            vertices[vertexOffset].y += sampleInfo.alpha * abcPointsB->get()[faceIndices->get()[currentIndex]].y;
            vertices[vertexOffset].z += sampleInfo.alpha * abcPointsB->get()[faceIndices->get()[currentIndex]].z;
          }
          vertexOffset++;
        }
        for(int j=2;j<5;j++)
        {
          currentIndex = indexOffset+(j%4);
          vertices[vertexOffset].x = iAlpha * abcPoints->get()[faceIndices->get()[currentIndex]].x;
          vertices[vertexOffset].y = iAlpha * abcPoints->get()[faceIndices->get()[currentIndex]].y;
          vertices[vertexOffset].z = iAlpha * abcPoints->get()[faceIndices->get()[currentIndex]].z;
          if(requiresInterpolation)
          {
            vertices[vertexOffset].x += sampleInfo.alpha * abcPointsB->get()[faceIndices->get()[currentIndex]].x;
            vertices[vertexOffset].y += sampleInfo.alpha * abcPointsB->get()[faceIndices->get()[currentIndex]].y;
            vertices[vertexOffset].z += sampleInfo.alpha * abcPointsB->get()[faceIndices->get()[currentIndex]].z;
          }
          vertexOffset++;
        }
      }
      indexOffset += count;
    }
    
    // load the normals
    Alembic::AbcGeom::IN3fGeomParam N = schema.getNormalsParam();
    if(N.valid())
    {
      Alembic::AbcGeom::N3fArraySamplePtr nsp = N.getExpandedValue(sampleInfo.floorIndex).getVals();
      Alembic::AbcGeom::N3fArraySamplePtr nspB;
      if(requiresInterpolation)
        nspB = N.getExpandedValue(sampleInfo.ceilIndex).getVals();
      
      if(nsp->size() > 0)
      {
        vertexOffset = 0;
        indexOffset = 0;
        for(size_t i=0;i<faceCounts->size();i++)
        {
          size_t count = faceCounts->get()[i];
          if(count == 3)
          {
            for(int j=0;j<3;j++)
            {
              currentIndex = indexOffset + j;
              normals[vertexOffset].x = iAlpha * nsp->get()[currentIndex].x;
              normals[vertexOffset].y = iAlpha * nsp->get()[currentIndex].y;
              normals[vertexOffset].z = iAlpha * nsp->get()[currentIndex].z;
              if(requiresInterpolation)
              {
                normals[vertexOffset].x += sampleInfo.alpha * nspB->get()[currentIndex].x;
                normals[vertexOffset].y += sampleInfo.alpha * nspB->get()[currentIndex].y;
                normals[vertexOffset].z += sampleInfo.alpha * nspB->get()[currentIndex].z;
                float length = normals[vertexOffset].x * normals[vertexOffset].x +
                               normals[vertexOffset].y * normals[vertexOffset].y +
                               normals[vertexOffset].z * normals[vertexOffset].z;
                if(length > 0.0f)
                {
                  length = sqrtf(length);
                  normals[vertexOffset].x /= length;
                  normals[vertexOffset].y /= length;
                  normals[vertexOffset].z /= length;
                }
              }
              vertexOffset++;
            }
          }
          else if(count == 4)
          {
            for(int j=0;j<3;j++)
            {
              currentIndex = indexOffset + j;
              normals[vertexOffset].x = iAlpha * nsp->get()[currentIndex].x;
              normals[vertexOffset].y = iAlpha * nsp->get()[currentIndex].y;
              normals[vertexOffset].z = iAlpha * nsp->get()[currentIndex].z;
              if(requiresInterpolation)
              {
                normals[vertexOffset].x += sampleInfo.alpha * nspB->get()[currentIndex].x;
                normals[vertexOffset].y += sampleInfo.alpha * nspB->get()[currentIndex].y;
                normals[vertexOffset].z += sampleInfo.alpha * nspB->get()[currentIndex].z;
                float length = normals[vertexOffset].x * normals[vertexOffset].x +
                               normals[vertexOffset].y * normals[vertexOffset].y +
                               normals[vertexOffset].z * normals[vertexOffset].z;
                if(length > 0.0f)
                {
                  length = sqrtf(length);
                  normals[vertexOffset].x /= length;
                  normals[vertexOffset].y /= length;
                  normals[vertexOffset].z /= length;
                }
              }
              vertexOffset++;
            }
            for(int j=2;j<5;j++)
            {
              currentIndex = indexOffset+(j%4);
              normals[vertexOffset].x = iAlpha * nsp->get()[currentIndex].x;
              normals[vertexOffset].y = iAlpha * nsp->get()[currentIndex].y;
              normals[vertexOffset].z = iAlpha * nsp->get()[currentIndex].z;
              if(requiresInterpolation)
              {
                normals[vertexOffset].x += sampleInfo.alpha * nspB->get()[currentIndex].x;
                normals[vertexOffset].y += sampleInfo.alpha * nspB->get()[currentIndex].y;
                normals[vertexOffset].z += sampleInfo.alpha * nspB->get()[currentIndex].z;
                float length = normals[vertexOffset].x * normals[vertexOffset].x +
                               normals[vertexOffset].y * normals[vertexOffset].y +
                               normals[vertexOffset].z * normals[vertexOffset].z;
                if(length > 0.0f)
                {
                  length = sqrtf(length);
                  normals[vertexOffset].x /= length;
                  normals[vertexOffset].y /= length;
                  normals[vertexOffset].z /= length;
                }
              }
              vertexOffset++;
            }
          }
          indexOffset += count;
        }
      }
    }

    if(!uvsLoaded)
    {
      Alembic::AbcGeom::IV2fGeomParam U = schema.getUVsParam();
      if(U.valid())
      {
        Alembic::AbcGeom::V2fArraySamplePtr usp = U.getExpandedValue(0).getVals();
        if(usp->size() > 0)
        {
          vertexOffset = 0;
          indexOffset = 0;
          for(size_t i=0;i<faceCounts->size();i++)
          {
            size_t count = faceCounts->get()[i];
            if(count == 3)
            {
              for(int j=0;j<3;j++)
              {
                currentIndex = indexOffset + j;
                uvs[vertexOffset].x = usp->get()[currentIndex].x;
                uvs[vertexOffset++].y = usp->get()[currentIndex].y;
              }
            }
            else if(count == 4)
            {
              for(int j=0;j<3;j++)
              {
                currentIndex = indexOffset + j;
                uvs[vertexOffset].x = usp->get()[currentIndex].x;
                uvs[vertexOffset++].y = usp->get()[currentIndex].y;
              }
              for(int j=2;j<5;j++)
              {
                currentIndex = indexOffset+(j%4);
                uvs[vertexOffset].x = usp->get()[currentIndex].x;
                uvs[vertexOffset++].y = usp->get()[currentIndex].y;
              }
            }
            indexOffset += count;
          }
        }
        uvsLoaded = true;
      }
    }
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICParsePointsCount(
  AlembicHandle &handle,
  KL::String & identifier,
  KL::Scalar & time,
  KL::Size & count
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if(archive == NULL)
    return;
    
  Alembic::AbcGeom::IPoints obj(getObjectFromArchive(archive,identifier.data()),Alembic::Abc::kWrapExisting);
  if( obj.valid() )
  {
    // get the schema
    Alembic::AbcGeom::IPointsSchema schema = obj.getSchema();

    // get the sample information
    SampleInfo sampleInfo = getSampleInfo(time,schema.getTimeSampling(),schema.getNumSamples());

    // get the sample
    Alembic::AbcGeom::IPointsSchema::Sample sample;
    schema.get(sample,sampleInfo.floorIndex);

    // access the points
    Alembic::Abc::P3fArraySamplePtr ptr = sample.getPositions();
    count = ptr->size();
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICParsePointsAttributes(
  AlembicHandle &handle,
  KL::String & identifier,
  KL::Scalar & time,
  KL::SlicedArray<KL::Vec3>& positions,
  KL::SlicedArray<KL::Quat>& orientations,
  KL::SlicedArray<KL::Scalar>& sizes,
  KL::SlicedArray<KL::Vec3>& scales,
  KL::SlicedArray<KL::Color>& colors
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if(archive == NULL)
    return;
    
  Alembic::AbcGeom::IPoints obj(getObjectFromArchive(archive,identifier.data()),Alembic::Abc::kWrapExisting);
  if( obj.valid() )
  {
    // get the schema
    Alembic::AbcGeom::IPointsSchema schema = obj.getSchema();

    // get the sample information
    SampleInfo sampleInfo = getSampleInfo(time,schema.getTimeSampling(),schema.getNumSamples());

    // get the sample
    Alembic::AbcGeom::IPointsSchema::Sample sample;
    schema.get(sample,sampleInfo.floorIndex);

    // get points and velocities
    Alembic::Abc::P3fArraySamplePtr positionsPtr = sample.getPositions();
    Alembic::Abc::V3fArraySamplePtr velocitiesPtr = sample.getVelocities();
    
    // store all of the points, and do proper interpolation
    if(positionsPtr->size() == positions.size())
    {
      bool validVelocities = velocitiesPtr != NULL;
      if(validVelocities)
        validVelocities = velocitiesPtr->size() == positionsPtr->size();
        
      if(sampleInfo.alpha != 0.0 && validVelocities)
      {
        for(KL::Size i=0;i<positions.size();i++)
        {
          positions[i].x = positionsPtr->get()[i].x + velocitiesPtr->get()[i].x * sampleInfo.alpha;
          positions[i].y = positionsPtr->get()[i].y + velocitiesPtr->get()[i].y * sampleInfo.alpha;
          positions[i].z = positionsPtr->get()[i].z + velocitiesPtr->get()[i].z * sampleInfo.alpha;
        }
      }
      else
      {
        for(KL::Size i=0;i<positions.size();i++)
        {
          positions[i].x = positionsPtr->get()[i].x;
          positions[i].y = positionsPtr->get()[i].y;
          positions[i].z = positionsPtr->get()[i].z;
        }
      }
    }
    
    // check if we can store the sizes
    Alembic::AbcGeom::IFloatGeomParam widthParam = schema.getWidthsParam();
    if(widthParam)
    {
      SampleInfo sampleInfo = getSampleInfo(time,widthParam.getTimeSampling(),widthParam.getNumSamples());
      if(widthParam.getNumSamples() > sampleInfo.floorIndex)
      {
        Alembic::Abc::FloatArraySamplePtr ptr = widthParam.getExpandedValue(sampleInfo.floorIndex).getVals();
        if(ptr != NULL)
        {
          for(KL::Size i=0;i<sizes.size();i++)
          {
            size_t index = i >= ptr->size() ? 0 : i;
            sizes[i] = ptr->get()[index];
          }
        }
      }
    }

    // check if we can store the colors
    if ( schema.getPropertyHeader( ".color" ) != NULL )
    {
      Alembic::Abc::IC4fArrayProperty prop = Alembic::Abc::IC4fArrayProperty( schema, ".color" );
      if(prop.valid())
      {
        SampleInfo sampleInfo = getSampleInfo(time,prop.getTimeSampling(),prop.getNumSamples());
        if(prop.getNumSamples() > sampleInfo.floorIndex)
        {
          Alembic::Abc::C4fArraySamplePtr ptr = prop.getValue(sampleInfo.floorIndex);
          if(ptr != NULL)
          {
            for(KL::Size i=0;i<colors.size();i++)
            {
              size_t index = i >= ptr->size() ? 0 : i;
              colors[i].r = ptr->get()[index].r;
              colors[i].g = ptr->get()[index].g;
              colors[i].b = ptr->get()[index].b;
              colors[i].a = ptr->get()[index].a;
            }
          }
        }
      }
    }
    
    // check if we can store the scales
    if ( schema.getPropertyHeader( ".scale" ) != NULL )
    {
      Alembic::Abc::IV3fArrayProperty prop = Alembic::Abc::IV3fArrayProperty( schema, ".scale" );
      if(prop.valid())
      {
        SampleInfo sampleInfo = getSampleInfo(time,prop.getTimeSampling(),prop.getNumSamples());
        if(prop.getNumSamples() > sampleInfo.floorIndex)
        {
          Alembic::Abc::V3fArraySamplePtr ptr = prop.getValue(sampleInfo.floorIndex);
          if(ptr != NULL)
          {
            for(KL::Size i=0;i<scales.size();i++)
            {
              size_t index = i >= ptr->size() ? 0 : i;
              scales[i].x = ptr->get()[index].x;
              scales[i].y = ptr->get()[index].y;
              scales[i].z = ptr->get()[index].z;
            }
          }
        }
      }
    }

    // check if we can store the scales
    if ( schema.getPropertyHeader( ".orientation" ) != NULL )
    {
      Alembic::Abc::IQuatfArrayProperty prop = Alembic::Abc::IQuatfArrayProperty( schema, ".orientation" );
      if(prop.valid())
      {
        SampleInfo sampleInfo = getSampleInfo(time,prop.getTimeSampling(),prop.getNumSamples());
        if(prop.getNumSamples() > sampleInfo.floorIndex)
        {
          Alembic::Abc::QuatfArraySamplePtr ptr = prop.getValue(sampleInfo.floorIndex);
          if(ptr != NULL)
          {
            if(ptr->size() == orientations.size())
            {
              for(KL::Size i=0;i<orientations.size();i++)
              {
                orientations[i].v.x = ptr->get()[i].v.x;
                orientations[i].v.y = ptr->get()[i].v.y;
                orientations[i].v.z = ptr->get()[i].v.z;
                orientations[i].w = ptr->get()[i].r;
              }
            }
          }
        }
      }
    }
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICParseCurvesUniforms(
  AlembicHandle &handle,
  KL::String & identifier,
  KL::VariableArray<KL::Integer>& indices
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if(archive == NULL)
    return;
    
  Alembic::AbcGeom::ICurves obj(getObjectFromArchive(archive,identifier.data()),Alembic::Abc::kWrapExisting);
  if( obj.valid() )
  {
    // get the schema
    Alembic::AbcGeom::ICurvesSchema schema = obj.getSchema();

    // get the sample
    Alembic::AbcGeom::ICurvesSchema::Sample sample;
    schema.get(sample,0);

    // get the number of vertices per curve
    Alembic::Abc::Int32ArraySamplePtr ptr = sample.getCurvesNumVertices();
    KL::Size count = 0;
    for(KL::Size i=0;i<ptr->size();i++)
      count += (ptr->get()[i] - 1) * 2;
      
    // now compute the indices
    indices.resize(count);
    KL::Size offset = 0;
    KL::Size pntOffset = 0;
    for(KL::Size i=0;i<ptr->size();i++)
    {
      for(KL::Size j=1;j<ptr->get()[i];j++)
      {
        indices[offset++] = pntOffset + j - 1;
        indices[offset++] = pntOffset + j;
      }
      pntOffset += ptr->get()[i];
    }
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICParseCurvesCount(
  AlembicHandle &handle,
  KL::String & identifier,
  KL::Size & count
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if(archive == NULL)
    return;

  Alembic::AbcGeom::ICurves obj(getObjectFromArchive(archive,identifier.data()),Alembic::Abc::kWrapExisting);
  if( obj.valid() )
  {
    // get the schema
    Alembic::AbcGeom::ICurvesSchema schema = obj.getSchema();

    // get the sample
    Alembic::AbcGeom::ICurvesSchema::Sample sample;
    schema.get(sample,0);

    // access the points
    Alembic::Abc::P3fArraySamplePtr ptr = sample.getPositions();
    count = ptr->size();
  }
}

FABRIC_EXT_EXPORT void FabricALEMBICParseCurvesAttributes(
  AlembicHandle &handle,
  KL::String & identifier,
  KL::Scalar & time,
  KL::SlicedArray<KL::Vec3>& vertices,
  KL::SlicedArray<KL::Scalar>& sizes,
  KL::Boolean & uvsLoaded,
  KL::SlicedArray<KL::Vec2>& uvs,
  KL::SlicedArray<KL::Color>& colors
  )
{
  Alembic::Abc::IArchive * archive = (Alembic::Abc::IArchive *)handle.pointer;
  if(archive == NULL)
    return;

  Alembic::AbcGeom::ICurves obj(getObjectFromArchive(archive,identifier.data()),Alembic::Abc::kWrapExisting);
  if( obj.valid() )
  {
    // get the schema
    Alembic::AbcGeom::ICurvesSchema schema = obj.getSchema();

    // get the sample information
    SampleInfo sampleInfo = getSampleInfo(time,schema.getTimeSampling(),schema.getNumSamples());

    // get the sample
    Alembic::AbcGeom::ICurvesSchema::Sample sample;
    schema.get(sample,sampleInfo.floorIndex);

    // get points and velocities
    Alembic::Abc::P3fArraySamplePtr positionsPtr = sample.getPositions();
    
    // store all of the points, and do proper interpolation
    if(positionsPtr->size() == vertices.size())
    {
      if(sampleInfo.alpha != 0.0)
      {
        Alembic::AbcGeom::ICurvesSchema::Sample sample2;
        schema.get(sample2,sampleInfo.ceilIndex);
        Alembic::Abc::P3fArraySamplePtr positionsPtr2 = sample2.getPositions();
        float ialpha = 1.0f - sampleInfo.alpha;
        
        for(KL::Size i=0;i<vertices.size();i++)
        {
          vertices[i].x = ialpha * positionsPtr->get()[i].x + sampleInfo.alpha * positionsPtr2->get()[i].x;
          vertices[i].y = ialpha * positionsPtr->get()[i].y + sampleInfo.alpha * positionsPtr2->get()[i].y;
          vertices[i].z = ialpha * positionsPtr->get()[i].z + sampleInfo.alpha * positionsPtr2->get()[i].z;
        }
      }
      else
      {
        for(KL::Size i=0;i<vertices.size();i++)
        {
          vertices[i].x = positionsPtr->get()[i].x;
          vertices[i].y = positionsPtr->get()[i].y;
          vertices[i].z = positionsPtr->get()[i].z;
        }
      }
    }

    if(!uvsLoaded)
    {
      Alembic::AbcGeom::IV2fGeomParam uvsParam = schema.getUVsParam();
      if(uvsParam)
      {
        if(uvsParam.getNumSamples() > 0)
        {
          Alembic::Abc::V2fArraySamplePtr ptr = uvsParam.getExpandedValue(0).getVals();
          if(ptr != NULL)
          {
            if(ptr->size() == uvs.size())
            {
              for(KL::Size i=0;i<uvs.size();i++)
              {
                uvs[i].x = ptr->get()[i].x;
                uvs[i].y = ptr->get()[i].y;
              }
            }
            else if(ptr->size() > 0)
            {
              Alembic::Abc::Int32ArraySamplePtr countPtr = sample.getCurvesNumVertices();
              if(ptr->size() == countPtr->size())
              {
                KL::Size offset = 0;
                KL::Size curveOffset = 0;
                for(KL::Size i=0;i<countPtr->size();i++)
                {
                  for(KL::Size j=0;j<countPtr->get()[i];j++)
                  {
                    uvs[offset].x = ptr->get()[curveOffset].x;
                    uvs[offset++].y = ptr->get()[curveOffset].y;
                  }
                  curveOffset++;
                }
              }
            }
          }
        }
      }
      if ( schema.getPropertyHeader( ".color" ) != NULL )
      {
        Alembic::Abc::IC4fArrayProperty prop = Alembic::Abc::IC4fArrayProperty( schema, ".color" );
        if(prop.valid())
        {
          if(prop.getNumSamples() > 0)
          {
            Alembic::Abc::C4fArraySamplePtr ptr = prop.getValue(0);
            if(ptr != NULL)
            {
              if(ptr->size() == colors.size())
              {
                for(KL::Size i=0;i<colors.size();i++)
                {
                  colors[i].r = ptr->get()[i].r;
                  colors[i].g = ptr->get()[i].g;
                  colors[i].b = ptr->get()[i].b;
                  colors[i].a = ptr->get()[i].a;
                }
              }
              else
              {
                Alembic::Abc::Int32ArraySamplePtr countPtr = sample.getCurvesNumVertices();
                if(ptr->size() == countPtr->size())
                {
                  KL::Size offset = 0;
                  KL::Size curveOffset = 0;
                  for(KL::Size i=0;i<countPtr->size();i++)
                  {
                    for(KL::Size j=0;j<countPtr->get()[i];j++)
                    {
                      colors[offset].r = ptr->get()[curveOffset].r;
                      colors[offset].g = ptr->get()[curveOffset].g;
                      colors[offset].b = ptr->get()[curveOffset].b;
                      colors[offset++].a = ptr->get()[curveOffset].a;
                    }
                    curveOffset++;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

