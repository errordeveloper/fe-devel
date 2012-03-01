/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

class Buffer
{
public:

  Buffer( void const *data, size_t size )
    : m_data( static_cast<uint8_t const *>( data ) )
    , m_size( size )
    , m_pos( 0 )
  {
  }
  
  inline bool skip( size_t n )
  {
    if( m_pos + n <= m_size )
    {
      m_pos += n;
      return true;
    }
    return false;
  }
  
  inline bool read( uint8_t   c[], size_t n )
  {
    if ( m_pos + n <= m_size )
    {
      memcpy( c, &m_data[m_pos], n );
      m_pos += n;
      return true;
    }
    else return false;
  }
  
private:
  uint8_t const *m_data;
  size_t m_size;
  size_t m_pos;
};

#if defined( _MSC_VER )
#pragma pack( 1 )
#endif
struct TGAHeader
{
  uint8_t   idLength;
  uint8_t   colorMapType;
  uint8_t   imageType;
  uint16_t  colorMapOffset;
  uint16_t  colorMapSize;
  uint8_t   colorMapBPP;
  uint16_t  imageOriginX;
  uint16_t  imageOriginY;
  uint16_t  imageWidth;
  uint16_t  imageHeight;
  uint8_t   imageBPP;
  uint8_t   imageDesc;
} 
#if defined( __GNUC__ )
__attribute__ ((packed))
#endif
;
#if defined( _MSC_VER )
#pragma pack( )
#endif

static inline void setPixelFromBytes(
  size_t     bpp,
  uint8_t     *v,
  KL::RGBA &p
  )
{
  switch( bpp )
  {
  case 2:
    p.r = (v[1] & 0x3C) << 1;
    p.g = ( (v[1] & 0x03) << 6 ) | ((v[0] & 0xE0) >> 2);
    p.b = (v[0] & 0x1F) << 3;
    
    // Double the bits so that white will become proper white 255 (and not 240)
    p.r |= p.r >> 5;
    p.g |= p.g >> 5;
    p.b |= p.b >> 5;
    
    p.a = (v[1] & 0x80) ? 255 : 0;
    break;
    
  case 3:
    p.r = v[2];
    p.g = v[1];
    p.b = v[0];
    p.a = 255;
    break;
    
  case 4:
    p.r = v[2];
    p.g = v[1];
    p.b = v[0];
    p.a = v[3];
    break;
  }
}

static void readTCPixelsRaw( 
  Buffer &buffer, 
  TGAHeader const &header, 
  KL::VariableArray<KL::RGBA> &imagePixels )
{
  size_t  bpp = header.imageBPP / 8;
  if( bpp < 2 || bpp > 4 )
    return;
  
  bool flipY = (header.imageDesc & 0x10) == 0;
  
  for( size_t y = 0; y < header.imageHeight; y++ )
  {
    size_t    yoff;
    if( flipY )
      yoff = (header.imageHeight - y - 1) * header.imageWidth;
    else
      yoff = y * header.imageWidth;

    for( size_t x = 0; x < header.imageWidth; x++ )
    {
      KL::RGBA &pixel = imagePixels[ x + yoff ];
      uint8_t     v[4];
      
      if( !buffer.read( v, bpp ) )
        return;
        
      setPixelFromBytes( bpp, v, pixel );
    }
  }
}

static void readTCPixelsRLE( 
  Buffer &buffer, 
  TGAHeader const &header, 
  KL::VariableArray<KL::RGBA> &imagePixels )
{
  size_t  bpp = header.imageBPP / 8;
  if( bpp < 2 || bpp > 4 )
    return;
  
  bool flipY = (header.imageDesc & 0x10) == 0;

  for( size_t y = 0; y < header.imageHeight; y++ )
  {
    size_t    yoff;
    if( flipY )
      yoff = (header.imageHeight - y - 1) * header.imageWidth;
    else
      yoff = y * header.imageWidth;
    for( size_t x = 0; x < header.imageWidth; )
    {
      uint8_t     rle, v[4];
      if( !buffer.read( &rle, 1 ) )
        return;
      
      if( rle & 0x80 )
      {
        // Repeated bytes
        size_t  count = (rle & 0x7F) + 1;
        
        if( !buffer.read( v, bpp ) )
          return;
          
        while( count-- )
        {
          if( x < header.imageWidth )
          {
            KL::RGBA &pixel = imagePixels[ x + yoff ];
            
            setPixelFromBytes( bpp, v, pixel );
          }
          x++;
        }
      }
      else
      {
        // Raw bytes
        size_t  count = (rle & 0x7F) + 1;
        
        while( count-- )
        {
          if( !buffer.read( v, bpp ) )
            return;
          
          if( x < header.imageWidth )
          {
            KL::RGBA &pixel = imagePixels[ x + yoff ];
            
            setPixelFromBytes( bpp, v, pixel );
          }
          x++;
        }
      }
    }
  }
}


FABRIC_EXT_EXPORT void FabricTGADecode(
  KL::Data tgaData,
  KL::Size tgaDataSize,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::VariableArray<KL::RGBA> &imagePixels
  )
{
  imageWidth = imageHeight = 0;
  imagePixels.resize( 0 );
  
  Buffer buffer( tgaData, tgaDataSize );
  
  TGAHeader   header;
  
  if( !buffer.read( (uint8_t *)&header, sizeof( header ) ) )
    return;
  
  // Skip over the ID field.
  if( !buffer.skip( header.idLength ) )
    return;
  
  imageWidth = header.imageWidth;
  imageHeight = header.imageHeight;
  
  imagePixels.resize( imageWidth * imageHeight );
  
  // No support for colormapped or grayscale 
  switch( header.imageType )
  {
  case 2:   // TrueColor uncompressed
    readTCPixelsRaw( buffer, header, imagePixels );
    break;
  case 10:  // TrueColor RLE
    readTCPixelsRLE( buffer, header, imagePixels );
    break;    
  }
}

FABRIC_EXT_EXPORT void FabricTGAOpenFileHandle(
  const KL::String & handle,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::VariableArray<KL::RGBA> &imagePixels
  )
{
  KL::FileHandleWrapper wrapper(handle);
  wrapper.ensureIsValidFile();
  FILE * fp= fopen(wrapper.getPath().data(),"rb");
  if(!fp)
  {
    Fabric::EDK::throwException("TGA Extension: File does not exist!");
    return;
  }
  
  fseek(fp, 0L, SEEK_END);
  KL::Size size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  KL::Data data = malloc(size);
  if(fread(data,1,size,fp) != size)
  {
    fclose(fp);
    Fabric::EDK::throwException("TGA Extension: Could not read file contents!");
    return;
  }
  fclose(fp);
  
  FabricTGADecode(data,size,imageWidth,imageHeight,imagePixels);
  free(data);
}
