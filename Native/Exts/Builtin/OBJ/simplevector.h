/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <math.h>

template<int Dim>
struct V
{
  float v[Dim];
};

struct V2: public V<2>
{
  V2(){}
  V2( float x, float y ){v[0]=x;v[1]=y;}
};

struct V3: public V<3>
{
  V3(){}
  V3( float x, float y, float z ){v[0]=x;v[1]=y;v[2]=z;}

  V3 Cross( const V3& other )const
  {
    return V3( v[1]*other.v[2] - v[2]*other.v[1], v[2]*other.v[0] - v[0]*other.v[2], v[0]*other.v[1] - v[1]*other.v[0] );
  }

  float length()const
  {
    return sqrtf( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
  }

  void Normalize()
  {
    float len = length();
    if( len != 0 )
    {
      float invLen = 1.0f / len;
      v[0] *= invLen;
      v[1] *= invLen;
      v[2] *= invLen;
    }
  }
};

inline V3 operator-( const V3& first, const V3& second )
{
  return V3( first.v[0]-second.v[0], first.v[1]-second.v[1], first.v[2]-second.v[2] );
}

inline V3 operator+( const V3& first, const V3& second )
{
  return V3( first.v[0]+second.v[0], first.v[1]+second.v[1], first.v[2]+second.v[2] );
}

inline V3 operator*( const V3& first, float factor )
{
  return V3( first.v[0]*factor, first.v[1]*factor, first.v[2]*factor );
}

