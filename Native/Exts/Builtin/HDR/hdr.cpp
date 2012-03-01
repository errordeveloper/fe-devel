/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>

#include <math.h>

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
  
  inline bool read( unsigned char c[], int n )
  {
    if ( m_pos + n <= m_size )
    {
      memcpy( c, &m_data[m_pos], n );
      m_pos += n;
      return true;
    }
    else return false;
  }
  
  inline bool readline( std::string &str )
  {
    if( m_pos == m_size )
      return false;
    
    size_t  m_start = m_pos;
    while( m_pos < m_size && m_data[m_pos] != '\n' )
      m_pos++;
    
    str = std::string( (char const *)m_data, m_start, (m_pos - m_start) );
    
    if( m_pos < m_size )
      m_pos++;
    
    return true;
  }
  
private:
  uint8_t const *m_data;
  size_t m_size;
  size_t m_pos;
};
  
static bool readHeader( 
  Buffer &hdr, 
  KL::Size &imageWidth, 
  KL::Size &imageHeight
  )
{
  std::string    str;
  
  if( !hdr.readline( str ) )
    return false;
  
  if( str[0] != '#' || str[1] != '?' )
    return false;
  
  bool foundFormat;
  for( ; ; )
  {
    if( !hdr.readline( str ) )
      return false;
      
    // Empty line signifies end of header
    if( str.empty() )
      break;
    
    if( str == "FORMAT=32-bit_rle_rgbe" )
      foundFormat = true;
  }
  if( !hdr.readline( str ) )
    return false;
  
  // Orientation + dimension specified. 
  // We currently only accept '-Y M +X n'
  int   w, h;
  if( sscanf( str.c_str(), "-Y %d +X %d", &h, &w ) != 2 )
    return false;
  
  imageWidth = (KL::Size)w;
  imageHeight = (KL::Size)h;
  
  return true;
}

static void rgbeToColor( unsigned char rgbe[4], KL::Color &col )
{
  if( rgbe[3] ) {
    float f = ldexp(1.0,rgbe[3]-(int)(128+8));
    col.r = rgbe[0] * f;
    col.g = rgbe[1] * f;
    col.b = rgbe[2] * f;
    col.a = 1.0;
  } else {
    col.r = col.g = col.b = 0.0;
    col.a = 1.0;
  }
}

static bool readPixelsRaw(
  Buffer  &hdr, 
  KL::VariableArray<KL::Color> &img,
  size_t  idx,
  size_t  nbPixels
  )
{
  unsigned char rgbe[4];
  
  while(nbPixels-- > 0) {
    if( !hdr.read( rgbe, 4 ) )
      return false;
    
    rgbeToColor( rgbe, img[idx] );
    idx++;
  }
  return true;
}

static bool readPixelsRLE(
  Buffer &hdr, 
  KL::VariableArray<KL::Color> &img,
  size_t scanlineWidth,
  size_t nbScanlines
  )
{
  if( scanlineWidth < 8 || scanlineWidth > 0x7FFF )
    return readPixelsRaw( hdr, img, 0, scanlineWidth * nbScanlines );
  
  std::vector<unsigned char>  scanline;
  scanline.resize( scanlineWidth * 4 );
  
  size_t    idx = 0;
  
  while( nbScanlines > 0 )
  {
    unsigned char rgbe[4];
    if( !hdr.read( rgbe, 4 ) )
      return false;
    if( rgbe[0] != 2 || rgbe[1] != 2 || rgbe[2] != 0x80 )
    {
      // Not an RLE file
      rgbeToColor( rgbe, img[idx] );
      return readPixelsRaw( hdr, img, idx + 1, scanlineWidth * nbScanlines - 1 );
    }
    
    if( ( (size_t(rgbe[2]) << 8 ) | size_t(rgbe[3])) != scanlineWidth )
      return false;
    
    unsigned char *scanlineStart, *scanlineEnd;
    
    scanlineStart = &scanline[ 0 ];
    for( int i = 0; i < 4; i++ )
    {
      scanlineEnd = &scanline[ (i+1) * scanlineWidth ];
      while( scanlineStart < scanlineEnd )
      {
        unsigned char rle[2];
        
        if( !hdr.read( rle, 2 ) )
          return false;
        
        if( rle[0] > 128 )
        {
          // Run of identical values
          size_t count = rle[0] - 128;
          if( count == 0 || count > ( size_t(scanlineEnd) - size_t(scanlineStart) ) )
            return false;
          while( count-- > 0 )
            *scanlineStart++ = rle[1];
        } else {
          // Run of non-identical values
          size_t count = rle[0];
          if( count == 0 || count > ( size_t(scanlineEnd) - size_t(scanlineStart) ) )
            return false;
          
          *scanlineStart++ = rle[1];
          if( -count > 0 )
          {
            if( !hdr.read( scanlineStart, count ) )
              return false;
            scanlineStart += count;
          }
        }
      }
    }
    
    for( size_t i = 0; i < scanlineWidth; i++ )
    {
      for( size_t j = 0; j < 4; j++ )
        rgbe[j] = scanline[i + scanlineWidth * j ];
      
      rgbeToColor( rgbe, img[ idx++ ] );
    }
    
    nbScanlines--;
  }
  return true;
}

FABRIC_EXT_EXPORT void FabricHDRDecode(
  KL::Data hdrData,
  KL::Size hdrDataSize,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::VariableArray<KL::Color> &imagePixels
  )
{
  Buffer buffer( hdrData, hdrDataSize );
  
  if( !readHeader( buffer, imageWidth, imageHeight ) )
    return;
  
  imagePixels.resize( imageWidth * imageHeight );
  
  readPixelsRLE( buffer, imagePixels, imageWidth, imageHeight );
}

FABRIC_EXT_EXPORT void FabricHDROpenFileHandle(
  const KL::String & handle,
  KL::Size &imageWidth,
  KL::Size &imageHeight,
  KL::VariableArray<KL::Color> &imagePixels
  )
{
  KL::FileHandleWrapper wrapper(handle);
  wrapper.ensureIsValidFile();
  FILE * fp= fopen(wrapper.getPath().data(),"rb");
  if(!fp)
  {
    Fabric::EDK::throwException("HDR Extension: File does not exist!");
    return;
  }
  
  fseek(fp, 0L, SEEK_END);
  KL::Size size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  KL::Data data = malloc(size);
  if(fread(data,1,size,fp) != size)
  {
    fclose(fp);
    Fabric::EDK::throwException("HDR Extension: Could not read file contents!");
    return;
  }
  fclose(fp);
  
  FabricHDRDecode(data,size,imageWidth,imageHeight,imagePixels);
  free(data);
}
