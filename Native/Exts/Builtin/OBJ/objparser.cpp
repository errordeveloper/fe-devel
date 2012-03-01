/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "objparser.h"
#include <sstream>
#include <set>

std::string ObjParser::kEmptyString;

class Exception
{
public:
  Exception(const char* msg)
    : m_msg(msg)
  {}

  const char* getMsg()
  {
    return m_msg.c_str();
  }

private:
  std::string m_msg;
};

void ParseExceptionThrow(const char* msg) {
  throw Exception( msg );
}

void(*g_cStrExceptionFunc)(const char*) = ParseExceptionThrow;

bool ObjParser::EntityKey::operator<(const ObjParser::EntityKey& other)const{
  int cmp = strcmp(m_objName.c_str(), other.m_objName.c_str());
  if(cmp)
    return cmp < 0;
  cmp = strcmp(m_groupName.c_str(), other.m_groupName.c_str());
  if(cmp)
    return cmp < 0;
  cmp = strcmp(m_matName.c_str(), other.m_matName.c_str());
  return cmp < 0;
}

bool IsSeparator( char c )
{
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

void skipToNextLine( std::istream& stream )
{
  char c = 0;
  while( stream.good() )
  {
    stream.get(c);
    if(c == '\n' || c == '\r')
    {
      char pc = stream.peek();
      if(pc != c && (pc == '\n' || pc == '\r'))
        stream.get(c);
      break;
    }
  }
}

bool parsingError( std::istream& stream )
{
  return !stream.good() || stream.fail();
}


bool skipSpaces( std::istream& stream )
{
  char c = 0;
  while( stream.good() )
  {
    char pc = stream.peek();
    if( pc == ' ' || pc == '\t' )
      stream.get(c);
    else
      break;
  }
  return c != 0;
}

bool skipExpectedString( std::istream& stream, const char* str, bool throwIfDifferent = true )
{
  while( true )
  {
    char c1 = str++[0], c2;
    if( c1 == 0 )
      break;
    stream.get( c2 );
    if( !stream.good() || c1 != c2 )
    {
      if(throwIfDifferent)
        ParseExceptionThrow( ("Expected token '" + std::string(str) + "' not found").c_str() );
      else
        return false;
    }
  }
  return true;
}

void skipMandatorySpaces( std::istream& stream )
{
  if( !skipSpaces(stream) )
    ParseExceptionThrow("space expected");
}

void ParseName( std::istream& stream, std::string& name ) {
  skipSpaces( stream );
  name.clear();
  while( stream.good() )
  {
    char c;
    stream.get(c);
    if(c == '\n' || c == '\r')
      break;
    name.push_back(c);
  }
}

template<int Dim>
void getV( V<Dim>& v, std::istream& stream )
{
  for( int i = 0; i < Dim; ++i )
  {
    skipMandatorySpaces( stream );
    stream >> v.v[i];
    if( parsingError( stream ) )
      ParseExceptionThrow("float value expected");
  }
}

void ObjParser::ParseV( std::istream& stream )
{
  V3 v;
  getV( v, stream );
  m_points.push_back( v );
  skipToNextLine( stream );// [jeromecg 20110728] Ignore 4D coords for now
}

void ObjParser::ParseN( std::istream& stream )
{
  V3 v;
  getV( v, stream );
  m_normals.push_back( v );
  skipToNextLine( stream );
}

void ObjParser::ParseT( std::istream& stream )
{
  V2 v;
  getV( v, stream );
  m_texCoords.push_back( v );
  skipToNextLine( stream );// [jeromecg 20110728] Ignore 3D coords for now
}

void ObjParser::ParseF( std::istream& stream )
{
  int nbFacePts = 0;
  int initalFacePt = (int)m_triangleIndices.size();
  if( m_sharedPointIndices.size() < m_points.size() )
    m_sharedPointIndices.resize( m_points.size() );

  while( true )
  {
    bool hadSpaces = skipSpaces( stream );
    char pc = stream.peek();
    if( pc == '\n' || pc == '\r' )
    {
      skipToNextLine( stream );
      break;
    }
    else if( !hadSpaces )
      ParseExceptionThrow("space expected");

    PointIndices indices;

    stream >> indices.m_point;
    if( parsingError( stream ) )
      ParseExceptionThrow("int value expected");
    if(indices.m_point < 0)
      indices.m_point += (int)m_points.size();
    else
      --indices.m_point;
    if( indices.m_point < 0 || indices.m_point > (int)m_points.size() )
      ParseExceptionThrow("out of range point index");

    char c = 0;
    stream.get(c);
    if(c == '/')
    {
      if( !IsSeparator( stream.peek() ) && stream.peek() != '/' )
      {
        stream >> indices.m_texCoord;
        if( parsingError( stream ) )
          ParseExceptionThrow("int value expected");
        if(indices.m_texCoord < 0)
          indices.m_texCoord += (int)m_texCoords.size();
        else
          --indices.m_texCoord;
        if( indices.m_texCoord < 0 || indices.m_texCoord > (int)m_texCoords.size() )
          ParseExceptionThrow("out of range texture index");
      }
      c = 0;
      stream.get(c);
      if(c == '/')
      {
        if( !IsSeparator( stream.peek() ) )
        {
          stream >> indices.m_normal;
          if( parsingError( stream ) )
            ParseExceptionThrow("int value expected");
          if(indices.m_normal < 0)
            indices.m_normal += (int)m_normals.size();
          else
            --indices.m_normal;
          if( indices.m_normal < 0 || indices.m_normal > (int)m_normals.size() )
            ParseExceptionThrow("out of range normal index");
        }
      }
      else
        stream.unget();
    }
    else
      stream.unget();

    // [jeromecg 20110728] Check if we need to unshare the vertex data
    // Note: this would be more complex (and might have to be done later) if we were taking smoothing groups into account...

    int ptIndex = indices.m_point;
    if( !m_sharedPointIndices[indices.m_point].isInitialized() )
    {
      m_sharedPointIndices[indices.m_point] = indices;
    }
    else if( m_sharedPointIndices[indices.m_point] != indices )
    {
      ptIndex = (int)m_sharedPointIndices.size();
      m_sharedPointIndices.push_back( indices );
    }

    nbFacePts++;

    if( nbFacePts <= 3 )
    {
      m_triangleIndices.push_back( ptIndex );
    }
    else
    {
      //Simple triangulation: 0 1 2 / 0 2 3 / 0 3 4...
      int first = m_triangleIndices[initalFacePt];
      int second = m_triangleIndices.back();
      m_triangleIndices.push_back( first );
      m_triangleIndices.push_back( second );
      m_triangleIndices.push_back( ptIndex );
    }
    if( nbFacePts >= 3 ) {
       if( m_currentEntityIter != m_entityIndices.end() ) {
         m_currentEntityIter->second.m_sharedFaces.push_back( (int)m_triangleMaterials.size() );
       }
       m_triangleSmoothingGroups.push_back( m_currentSmoothingGroup );
       m_triangleMaterials.push_back( m_currentMaterialIndex );
    }
  }
  if( nbFacePts < 3 )
    ParseExceptionThrow("face has less than 3 points");
}

int ObjParser::ParseS( std::istream& stream )
{
  skipMandatorySpaces( stream );
  char pc = stream.peek();
  if( pc == 'o')
  {
    skipExpectedString( stream, "off" );
    return -1;
  }
  else
  {
    int smoothingGroup;
    stream >> smoothingGroup;
    if( parsingError( stream ) )
      ParseExceptionThrow("int value expected for smoothing group");
    return smoothingGroup;
  }
}

void ObjParser::ComputeMissingNormals()
{
  //Quick check if any normal is missing
  size_t i, j;
  for( i = 0; i < m_triangleIndices.size(); ++i )
  {
    if( m_sharedPointIndices[ m_triangleIndices[i] ].m_normal == INT_MAX )
      break;
  }
  if( i == m_triangleIndices.size() )//This covers the nbPoints == 0 case too
    return;

  // Compute triangle normals
  size_t nbTriangles = m_triangleIndices.size() / 3;
  std::vector< V3 > triangleNormals;
  triangleNormals.resize( nbTriangles );

  for( i = 0; i < nbTriangles; ++i )
  {
    V3 v1 = m_points[ m_sharedPointIndices[ m_triangleIndices[ i*3+1 ] ].m_point ] - m_points[ m_sharedPointIndices[ m_triangleIndices[ i*3 ] ].m_point ];
    V3 v2 = m_points[ m_sharedPointIndices[ m_triangleIndices[ i*3+2 ] ].m_point ] - m_points[ m_sharedPointIndices[ m_triangleIndices[ i*3 ] ].m_point ];
    triangleNormals[i] = v1.Cross( v2 );
    triangleNormals[i].Normalize();
  }

  bool hasSmoothingGroups = false;
  for( i = 0; i < m_triangleIndices.size(); ++i )
  {
    if( m_triangleSmoothingGroups[i] != -1 )
    {
      hasSmoothingGroups = true;
      break;
    }
  }

  if( !hasSmoothingGroups )
  {
    //Optimize this simpler case
    m_normals.resize( m_points.size(), V3( 0, 0, 0 ) );

    for( i = 0; i < m_triangleIndices.size(); ++i )
    {
      int ptIndex = m_sharedPointIndices[ m_triangleIndices[i] ].m_point;
      m_normals[ptIndex] = m_normals[ptIndex] + triangleNormals[i/3];
    }

    for( i = 0; i < m_normals.size(); ++i )
    {
      m_normals[i].Normalize();
      m_sharedPointIndices[i].m_normal = i;
    }
  }
  else
  {
    // Note: for simplicity, we only maintain vertex-based sharing (no per face sharing)
    size_t nbPoints = m_points.size();
    size_t nbFacePoints = m_triangleIndices.size();

    //Take smoothing groups into account: build a point->face mapping (in a CPU and memory efficient way)
    std::vector< int > nbFacesPerPoint;
    nbFacesPerPoint.resize( nbPoints, 0 );

    for( i = 0; i < nbFacePoints; ++i )
      nbFacesPerPoint[ m_sharedPointIndices[ m_triangleIndices[ i ] ].m_point ]++;

    std::vector< int > pointFacesOffset;
    pointFacesOffset.resize( nbFacesPerPoint.size() );
    pointFacesOffset[0] = 0;
    for( i = 1; i < nbPoints; ++i )
      pointFacesOffset[i] = pointFacesOffset[i-1] + nbFacesPerPoint[i-1];

    std::vector< int > facePointsPerPoint;
    facePointsPerPoint.resize( nbFacePoints );
    for( i = 0; i < nbFacePoints; ++i )
      facePointsPerPoint[ pointFacesOffset[ m_sharedPointIndices[ m_triangleIndices[ i ] ].m_point ]++ ] = i;

    //Average normals per smoothing groups
    std::set<int> smoothGroups;
    size_t nbOriginalNormals = m_normals.size();
    size_t facesOffset = 0;
    for( i = 0; i < nbPoints; ++i )
    {
      size_t nbFaces = nbFacesPerPoint[i];
      for( j = 0; j < nbFaces; ++j )
      {
        int faceIndex = facePointsPerPoint[facesOffset+j]/3;
        smoothGroups.insert( m_triangleSmoothingGroups[faceIndex] );
      }

      for( std::set<int>::const_iterator it = smoothGroups.begin(); it != smoothGroups.end(); ++it )
      {
        int normalIndex = -1;
        for( j = 0; j < nbFaces; ++j )
        {
          int facePoint = facePointsPerPoint[facesOffset+j];
          int face = facePoint/3;
          if( m_triangleSmoothingGroups[ face ] == *it )
          {
            int faceSharedPointIndex = m_triangleIndices[facePoint];
            bool keepOriginalNormal = false;
            int prevNormalIndex = m_sharedPointIndices[ faceSharedPointIndex ].m_normal;
            if( prevNormalIndex != INT_MAX )
            {
              if( prevNormalIndex < int(nbOriginalNormals) )//Don't overwrite originally specified normals even if in a smoothing group
                keepOriginalNormal = true;
              else if( prevNormalIndex != normalIndex )//We need to split the point indices in order to preserve the existing normal
              {
                size_t nbPtIndices = m_sharedPointIndices.size();
                m_sharedPointIndices.resize( nbPtIndices+1 );
                m_sharedPointIndices[ nbPtIndices ] = m_sharedPointIndices[ faceSharedPointIndex ];
                faceSharedPointIndex = m_triangleIndices[facePoint] = nbPtIndices;
              }
            }
            if( !keepOriginalNormal )
            {
              if( normalIndex == -1 )
              {
                normalIndex = m_normals.size();
                m_normals.resize( normalIndex+1, V3(0,0,0) );
              }
              m_normals[normalIndex] = m_normals[normalIndex] + triangleNormals[ face ];
              m_sharedPointIndices[ faceSharedPointIndex ].m_normal = normalIndex;
            }
          }
        }
        if( normalIndex != -1 )
          m_normals[normalIndex].Normalize();
      }
      smoothGroups.clear();
      facesOffset += nbFaces;
    }
  }
}

ObjParser::ObjParser( std::istream& stream, bool splitObjs, bool splitGroups, bool splitMaterials, void(*cStrExceptionFunc)(const char*) )
  : m_currentSmoothingGroup(-1),
    m_currentMaterialIndex(-1)
{
  if(cStrExceptionFunc)
    g_cStrExceptionFunc = cStrExceptionFunc;

  // [jeromecg 20110728] Right now this parser is very simple as it agglomerates all points and faces in 1 big object,
  // ignoring materials, groups, smoothing groups, etc.
  int lineCount = 0;

  EntityKey currEntityKey;
  m_currentEntityIter = m_entityIndices.end();
  bool entityKeyChanged = false;

  try
  {
    while( stream.good() )
    {
      char c = 0;
      stream.get(c);
      switch( c )
      {
      case '\n':
      case '\r':
      case ' ':
      case '\t':
        break;
      case 'v':
        {
          char nextC;
          stream.get(nextC);
          switch( nextC )
          {
          case 'p':
            skipToNextLine( stream );//patch: ignore
            break;
          case 'n':
            ParseN( stream );
            break;
          case 't':
            ParseT( stream );
            break;
          default:
            stream.unget();
            ParseV( stream );
          }
          break;
        }
      case 'f':
        if(entityKeyChanged) {
          m_currentEntityIter = m_entityIndices.insert( std::make_pair( currEntityKey, EntityIndices() ) ).first;
          entityKeyChanged = false;
        }
        ParseF( stream );
        break;
      case 's':
        m_currentSmoothingGroup = ParseS( stream );
        break;
      case 'o':
        if( splitObjs ) {
          ParseName( stream, currEntityKey.m_objName );
          entityKeyChanged = true;
          break;
        }
        skipToNextLine( stream );
        break;
      case 'g':
        if( splitGroups ) {
          ParseName( stream, currEntityKey.m_groupName );
          entityKeyChanged = true;
          break;
        }
        skipToNextLine( stream );
        break;
      case 'u':
        if( skipExpectedString( stream, "semtl", false ) ) {
          std::string matName;
          ParseName( stream, matName );

          std::pair< std::map< std::string, int >::iterator, bool > matIter = m_materialIndices.insert( std::make_pair( matName, (int)m_materials.size() ) );
          if( matIter.second == true ) {
            m_materials.push_back( matName );
          }
          m_currentMaterialIndex = matIter.first->second;

          if( splitMaterials ) {
            currEntityKey.m_matName = matName;
            entityKeyChanged = true;
          }
          break;
        }
        skipToNextLine( stream );
        break;
      case 'm':
        if( skipExpectedString( stream, "tllib", false ) ) {
          std::string matLibName;
          ParseName( stream, matLibName );
          m_materialLibs.push_back(matLibName);
        }
        skipToNextLine( stream );
        break;
      default:
        skipToNextLine( stream );
      }
      ++lineCount;
    }
    ComputeMissingNormals();

    m_entities.reserve( m_entityIndices.size() );
    for( EntityFacesMap::const_iterator entityIter = m_entityIndices.begin(); entityIter != m_entityIndices.end(); ++entityIter )
      m_entities.push_back( entityIter );

    if( m_entities.size() > 1 ) {
      //Define entity-relative point indices, and maintain point sharing

      std::vector<int> localPtIndices;
      localPtIndices.resize( m_sharedPointIndices.size(), -1 );

      for( EntityFacesMap::iterator entityIter = m_entityIndices.begin(); entityIter != m_entityIndices.end(); ++entityIter ) {
        EntityIndices& entityIndices = entityIter->second;
        entityIndices.m_sharedPointIndices.reserve( entityIndices.m_sharedFaces.size()/2 );
        entityIndices.m_localFacePoints.reserve( entityIndices.m_sharedFaces.size()*3 );

        std::vector<int>::const_iterator iter;
        for( std::vector<int>::const_iterator iter = entityIndices.m_sharedFaces.begin(); iter != entityIndices.m_sharedFaces.end(); ++iter ) {
          int startIndex = (*iter)*3;
          int endIndex = startIndex+3;
          for( int i = startIndex; i < endIndex; ++i ) {
            int sharedPointIndex = m_triangleIndices[i];

            int localPointIndex = localPtIndices[sharedPointIndex];
            if( localPointIndex == -1 ) {
              localPointIndex = (int)entityIndices.m_sharedPointIndices.size();
              localPtIndices[sharedPointIndex] = localPointIndex;
              entityIndices.m_sharedPointIndices.push_back( sharedPointIndex );
            }
            entityIndices.m_localFacePoints.push_back( localPointIndex );
          }
        }
        //clear marks
        for( std::vector<int>::const_iterator iter = entityIndices.m_sharedPointIndices.begin(); iter != entityIndices.m_sharedPointIndices.end(); ++iter )
          localPtIndices[*iter] = -1;
      }
    }
  }
  catch ( Exception e )
  {
    std::stringstream sstr;
    sstr << "Error while parsing .obj file at line " << lineCount << ": " << e.getMsg();
    m_errors.push_back( sstr.str() );
  }
  catch ( ... )
  {
    std::stringstream sstr;
    sstr << "Unexpected error while parsing .obj file at line " << lineCount;
    m_errors.push_back( sstr.str() );
  }
}

void ObjParser::CheckEntityIndex( int entity )const {
  if(entity != -1 && entity != 0 && (size_t)entity >= m_entities.size())
    (*g_cStrExceptionFunc)("out of range entity");
}

const std::string& ObjParser::GetEntityObjectName( int entity )const {
  CheckEntityIndex(entity);
  if(entity == -1) {
     if(m_entities.size() == 1)
       entity = 0;
     else
       return kEmptyString;
  }
  if(entity == 0 && m_entities.empty())
    return kEmptyString;
  return m_entities[entity]->first.m_objName;
}

const std::string& ObjParser::GetEntityGroupName( int entity )const{
  CheckEntityIndex(entity);
  if(entity == -1) {
     if(m_entities.size() == 1)
       entity = 0;
     else
       return kEmptyString;
  }
  if(entity == 0 && m_entities.empty())
    return kEmptyString;
  return m_entities[entity]->first.m_groupName;
}

const std::string& ObjParser::GetEntityMaterialName( int entity )const{
  CheckEntityIndex(entity);
  if(entity == -1) {
     if(m_entities.size() == 1)
       entity = 0;
     else
       return kEmptyString;
  }
  if(entity == 0 && m_entities.empty())
    return kEmptyString;
  return m_entities[entity]->first.m_matName;
}

size_t ObjParser::GetNbEntityPoints( int entity )const
{
  CheckEntityIndex(entity);
  if( m_entities.size() <= 1 || entity == -1 )
    return m_sharedPointIndices.size();

  return m_entities[entity]->second.m_sharedPointIndices.size();
}

V3 ObjParser::GetEntityPoint(int entity, int ptIndex)const
{
  if( m_entities.size() > 1 && entity != -1 )
    ptIndex = m_entities[entity]->second.m_sharedPointIndices[ptIndex];

  int index = m_sharedPointIndices[ptIndex].m_point;
  if( index == INT_MAX )
    return V3( 0, 0, 0 );
  else
    return m_points[ index ];
}

V3 ObjParser::GetEntityNormal(int entity, int ptIndex)const
{
  if( m_entities.size() > 1 && entity != -1 )
    ptIndex = m_entities[entity]->second.m_sharedPointIndices[ptIndex];

  int index = m_sharedPointIndices[ptIndex].m_normal;
  if( index == INT_MAX )
    return V3( 0, 1, 0 );
  else
    return m_normals[ index ];
}

V2 ObjParser::GetEntityTextureCoord(int entity, int ptIndex)const
{
  if( m_entities.size() > 1 && entity != -1 )
    ptIndex = m_entities[entity]->second.m_sharedPointIndices[ptIndex];

  int index = m_sharedPointIndices[ptIndex].m_texCoord;
  if( index == INT_MAX )
    return V2( 0, 0 );
  else
    return m_texCoords[ index ];
}

size_t ObjParser::GetNbEntityTriangles( int entity )const {
  CheckEntityIndex(entity);
  if( m_entities.size() <= 1 || entity == -1 )
    return m_triangleMaterials.size();

  return m_entities[entity]->second.m_sharedFaces.size();
}

void ObjParser::GetEntityTriangleIndices(int entity, int triIndex, int& i1, int& i2, int& i3)const
{
  if( m_entities.size() <= 1 || entity == -1 ) {
    i1 = m_triangleIndices[triIndex*3];
    i2 = m_triangleIndices[triIndex*3+1];
    i3 = m_triangleIndices[triIndex*3+2];
  } else {
    const std::vector< int >& facePoints = m_entities[entity]->second.m_localFacePoints;
    i1 = facePoints[triIndex*3];
    i2 = facePoints[triIndex*3+1];
    i3 = facePoints[triIndex*3+2];
  }
}

int ObjParser::GetEntityTriangleMaterialIndex(int entity, int triIndex)const
{
  if( m_entities.size() > 1 && entity != -1 )
    triIndex = m_entities[entity]->second.m_sharedFaces[triIndex];

  return m_triangleMaterials[triIndex];
}
