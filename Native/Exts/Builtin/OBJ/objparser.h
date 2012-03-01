/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <string.h>
#include "simplevector.h"
#include <limits.h>

class ObjParser
{
public:
  ObjParser( std::istream& stream, bool splitObjs, bool splitGroups, bool splitMaterials, void(*cStrExceptionFunc)(const char*) = NULL );

  size_t GetNbErrors()const{ return m_errors.size(); }
  const std::string& GetError( size_t index )const{ return m_errors[index]; }

  bool HasTextureCoords()const{ return !m_texCoords.empty(); }

  size_t GetNbMaterialLibraries()const{ return m_materialLibs.size(); }
  const std::string& GetMaterialLibrary( size_t index )const{ return m_materialLibs[index]; }

  size_t GetNbMaterials()const{ return m_materials.size(); }
  const std::string& GetMaterial( size_t index )const{ return m_materials[index]; }

  size_t GetNbEntities()const{ return m_entities.empty() ? 1 : m_entities.size(); }
  const std::string& GetEntityObjectName( int entity )const;
  const std::string& GetEntityGroupName( int entity )const;
  const std::string& GetEntityMaterialName( int entity )const;

  size_t GetNbEntityPoints( int entity )const;

  V3 GetEntityPoint(int entity, int ptIndex)const;
  V3 GetEntityNormal(int entity, int ptIndex)const;
  V2 GetEntityTextureCoord(int entity, int ptIndex)const;

  size_t GetNbEntityTriangles( int entity )const;

  void GetEntityTriangleIndices(int entity, int triIndex, int& i1, int& i2, int& i3)const;
  int GetEntityTriangleMaterialIndex(int entity, int triIndex)const;//-1 if none

private:
  void ParseV( std::istream& stream );
  void ParseN( std::istream& stream );
  void ParseT( std::istream& stream );
  void ParseF( std::istream& stream );
  int ParseS( std::istream& stream );

  void ComputeMissingNormals();
  void CheckEntityIndex( int entity )const;

  std::vector<std::string> m_errors;

  std::vector<V3> m_points, m_normals;
  std::vector<V2> m_texCoords;

  struct PointIndices
  {
    PointIndices()
      : m_point(INT_MAX)
      , m_normal(INT_MAX)
      , m_texCoord(INT_MAX)
    {}

    bool isInitialized()
    {
      return m_point != INT_MAX;
    }

    bool operator==( const PointIndices& other )const
    {
      return m_point == other.m_point && m_normal == other.m_normal && m_texCoord == other.m_texCoord;
    }
    bool operator!=( const PointIndices& other )const
    {
      return !(*this == other);
    }

    int m_point;
    int m_normal;
    int m_texCoord;
  };

  std::vector<std::string> m_materials;
  std::vector<std::string> m_materialLibs;

  std::vector<int> m_triangleSmoothingGroups;
  std::vector<int> m_triangleMaterials;
  std::vector<int> m_triangleIndices;
  std::vector<PointIndices> m_sharedPointIndices;

  struct EntityKey {
    std::string m_objName;
    std::string m_groupName;
    std::string m_matName;

    bool operator<(const EntityKey& other)const;
  };
  struct EntityIndices {
    std::vector< int > m_sharedFaces;
    std::vector< int > m_sharedPointIndices;
    std::vector< int > m_localFacePoints;
  };

  typedef std::map<EntityKey, EntityIndices > EntityFacesMap;
  EntityFacesMap m_entityIndices;
  std::vector< EntityFacesMap::const_iterator > m_entities;

  std::map< std::string, int > m_materialIndices;

  int m_currentSmoothingGroup;
  int m_currentMaterialIndex;
  EntityFacesMap::iterator m_currentEntityIter;
  static std::string kEmptyString;
};
