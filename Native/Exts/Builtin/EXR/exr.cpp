/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>

#include <ImfIO.h>
#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <ImathBox.h>

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

class MemIStream : public Imf::IStream
{
public:

  MemIStream( void const *data, size_t size )
    : IStream( "<in memory stream>" )
    , m_data( static_cast<uint8_t const *>( data ) )
    , m_size( size )
    , m_pos( 0 )
  {
  }
  
  virtual bool read( char c[], int n )
  {
    if ( m_pos + n <= m_size )
    {
      memcpy( c, &m_data[m_pos], n );
      m_pos += n;
      return true;
    }
    else return false;
  }
  
  virtual Imf::Int64 tellg()
  {
    return m_pos;
  }
  
  virtual void seekg( Imf::Int64 pos )
  {
    m_pos = pos;
  }
  
  virtual void clear()
  {
  }
  
private:

  uint8_t const *m_data;
  size_t m_size;
  size_t m_pos;
};
  
class FileIStream : public Imf::IStream
{
public:

  FileIStream( const KL::String & filename )
    : IStream( "<file stream>" )
    , m_pos( 0 )
    , m_size( 0 )
  {
    m_file = fopen(filename.data(),"rb");
    if(!m_file)
    {
      Fabric::EDK::throwException("  { FabricEXR } : Cannot open file: '%s'",filename.data());
      return;
    }
    fseek(m_file, 0L, SEEK_END);
    m_size = ftell(m_file);
    fseek(m_file, 0L, SEEK_SET);
  }
  
  ~FileIStream()
  {
    clear();
  }
  
  virtual bool read( char c[], int n )
  {
    if ( m_pos + n <= m_size )
    {
      if(fread(c,1,n,m_file) != n)
      {
        Fabric::EDK::throwException("  { FabricEXR } : Unexpected failure while reading.");
        return false;
      }
      m_pos += n;
      return true;
    }
    else return false;
  }
  
  virtual Imf::Int64 tellg()
  {
    return m_pos;
  }
  
  virtual void seekg( Imf::Int64 pos )
  {
    fseek(m_file, pos, SEEK_SET);
    m_pos = pos;
  }
  
  virtual void clear()
  {
    if(m_file != NULL)
    {
      fclose(m_file);
      m_file = NULL;
    }
  }
  
private:

  FILE * m_file;
  size_t m_pos;
  size_t m_size;
};

FABRIC_EXT_EXPORT void FabricEXRDecode(
  KL::Data exrData,
  KL::Size exrDataSize,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::VariableArray<KL::Color> &imagePixels
  )
{
  MemIStream memIStream( exrData, exrDataSize );
  Imf::RgbaInputFile rgbaInputFile( memIStream );
  Imath::Box2i dw = rgbaInputFile.dataWindow();
  imageWidth = dw.max.x - dw.min.x + 1;
  imageHeight = dw.max.y - dw.min.y + 1;
  Imf::Array2D<Imf::Rgba> rgbaPixels;
  rgbaPixels.resizeErase( imageHeight, imageWidth );
  rgbaInputFile.setFrameBuffer( &rgbaPixels[0][0] - dw.min.x - dw.min.y * imageWidth, 1, imageWidth );
  rgbaInputFile.readPixels( dw.min.y, dw.max.y );
  
  imagePixels.resize( imageWidth*imageHeight );
  size_t index = 0;
  for ( size_t y=0; y<imageHeight; ++y )
  {
    for ( size_t x=0; x<imageWidth; ++x )
    {
      Imf::Rgba const &rgba = rgbaPixels[y][x];
      KL::Color &klColor = imagePixels[index++];
      klColor.r = rgba.r;
      klColor.g = rgba.g;
      klColor.b = rgba.b;
      klColor.a = rgba.a;
    }
  }
}

FABRIC_EXT_EXPORT void FabricEXROpenFileHandle(
  const KL::String & handle,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::VariableArray<KL::Color> &imagePixels
  )
{
  KL::FileHandleWrapper wrapper(handle);
  wrapper.ensureIsValidFile();
  FileIStream filestream(wrapper.getPath());
  Imf::RgbaInputFile rgbaInputFile( filestream );
  Imath::Box2i dw = rgbaInputFile.dataWindow();
  imageWidth = dw.max.x - dw.min.x + 1;
  imageHeight = dw.max.y - dw.min.y + 1;
  Imf::Array2D<Imf::Rgba> rgbaPixels;
  rgbaPixels.resizeErase( imageHeight, imageWidth );
  rgbaInputFile.setFrameBuffer( &rgbaPixels[0][0] - dw.min.x - dw.min.y * imageWidth, 1, imageWidth );
  rgbaInputFile.readPixels( dw.min.y, dw.max.y );
  
  imagePixels.resize( imageWidth*imageHeight );
  size_t index = 0;
  for ( size_t y=0; y<imageHeight; ++y )
  {
    for ( size_t x=0; x<imageWidth; ++x )
    {
      Imf::Rgba const &rgba = rgbaPixels[y][x];
      KL::Color &klColor = imagePixels[index++];
      klColor.r = rgba.r;
      klColor.g = rgba.g;
      klColor.b = rgba.b;
      klColor.a = rgba.a;
    }
  }
}
