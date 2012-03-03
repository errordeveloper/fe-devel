/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Clients/NPAPI/Darwin/WindowlessCGViewPort.h>
#include <Fabric/Clients/NPAPI/Interface.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/Event.h>
#include <Fabric/Core/MT/LogCollector.h>

namespace Fabric
{
  namespace NPAPI
  {
    RC::Handle<ViewPort> WindowlessCGViewPort::Create( RC::ConstHandle<Interface> const &interface )
    {
      NPP npp = interface->getNPP();
      
      if ( NPN_SetValue( npp, NPPVpluginWindowBool, (void *)false ) != NPERR_NO_ERROR )
        return 0;
        
      if ( NPN_SetValue( npp, NPPVpluginTransparentBool, (void *)true ) != NPERR_NO_ERROR )
        return 0;

      NPBool supportsCarbonEventModel;
#if !defined( __x86_64 )
      if ( NPN_GetValue( npp, NPNVsupportsCarbonBool, &supportsCarbonEventModel ) != NPERR_NO_ERROR )
#endif
        supportsCarbonEventModel = false;

      NPBool supportsCocoaEventModel;
      if ( NPN_GetValue( npp, NPNVsupportsCocoaBool, &supportsCocoaEventModel ) != NPERR_NO_ERROR )
        supportsCocoaEventModel = false;

      if ( !supportsCarbonEventModel )
        return 0;
#if !defined( __x86_64 )
      if ( NPN_SetValue( npp, NPPVpluginEventModel, (void *)NPEventModelCarbon ) != NPERR_NO_ERROR )
        return 0;
#endif
        
      NPBool supportsCoreGraphics;
      if ( NPN_GetValue( npp, NPNVsupportsCoreGraphicsBool, &supportsCoreGraphics ) != NPERR_NO_ERROR )
        supportsCoreGraphics = false;
      
      if ( !supportsCoreGraphics )
        return 0;
      if ( NPN_SetValue( npp, NPPVpluginDrawingModel, (void *)NPDrawingModelCoreGraphics ) != NPERR_NO_ERROR )
        return 0;
      
      FABRIC_LOG( "Using CoreGraphics drawing model" );
      
      return new WindowlessCGViewPort( interface );
    }
    
    WindowlessCGViewPort::WindowlessCGViewPort( RC::ConstHandle<Interface> const &interface )
      : ViewPort( interface )
      , m_npp( interface->getNPP() )
      , m_cgContext( 0 )
      , m_windowRef( 0 )
      , m_windowWidth( 0 )
      , m_windowHeight( 0 )
    {
    }
    
    WindowlessCGViewPort::~WindowlessCGViewPort()
    {
      if ( m_windowWidth && m_windowHeight )
      {
        CGImageRelease( m_offscreenCGImage );
        free( m_offscreenData );
      }
      
      if ( m_windowRef )
        aglDestroyContext( m_windowAGLContext );
      
      if ( m_cgContext )
        CGContextRelease( m_cgContext );
    }
      
    void WindowlessCGViewPort::needsRedraw()
    {
      NPRect invalidRect;
      invalidRect.left = m_windowLeft;
      invalidRect.top = m_windowTop;
      invalidRect.right = m_windowLeft+m_windowWidth;
      invalidRect.bottom = m_windowTop+m_windowHeight;
      NPN_InvalidateRect( m_npp, &invalidRect );
    }
    
    NPError WindowlessCGViewPort::nppSetWindow( NPWindow *npWindow )
    {
      NP_CGContext *npCGContext = reinterpret_cast<NP_CGContext *>( npWindow->window );
      if ( !npCGContext )
      {
        static NP_CGContext nullNPCGContext = { 0, 0 };
        npCGContext = &nullNPCGContext;
      }
      
      if ( m_cgContext != npCGContext->context )
      {
        if ( m_cgContext )
          CGContextRelease( m_cgContext );
        
        m_cgContext = npCGContext->context;
        
        if ( m_cgContext )
          CGContextRetain( m_cgContext );
      }
      
      if ( m_windowRef != npCGContext->window )
      {
        if ( m_windowRef )
          aglDestroyContext( m_windowAGLContext );
        
        m_windowRef = (WindowRef)npCGContext->window;
        
        if ( m_windowRef )
        {
          GLint pixelFormatAttributes[] =
          {
            AGL_RGBA,
            AGL_DEPTH_SIZE, 16,
            AGL_NONE
          };
          
          AGLPixelFormat aglPixelFormat = aglCreatePixelFormat( pixelFormatAttributes );
          FABRIC_ASSERT( aglPixelFormat );

          m_windowAGLContext = aglCreateContext( aglPixelFormat, NULL );
          FABRIC_ASSERT( m_windowAGLContext );
          aglDestroyPixelFormat( aglPixelFormat );
          
          FABRIC_VERIFY( aglSetWindowRef( m_windowAGLContext, m_windowRef ) );
        }
      }

      m_windowLeft = npWindow->x;
      m_windowTop = npWindow->y;
      
      if ( m_windowWidth != npWindow->width || m_windowHeight != npWindow->height )
      {
        if ( m_windowWidth && m_windowHeight )
        {
          CGImageRelease( m_offscreenCGImage );
          free( m_offscreenData );
        }
      
        m_windowWidth = npWindow->width;
        m_windowHeight = npWindow->height;
      
        if ( m_windowWidth && m_windowHeight )
        {
          size_t offscreenDataSize = m_windowWidth * m_windowHeight * 4;
          
          m_offscreenData = malloc( offscreenDataSize );
          FABRIC_ASSERT( m_offscreenData );
          
          CGDataProviderRef cgDataProvider = CGDataProviderCreateWithData( 0, m_offscreenData, offscreenDataSize, 0 );
          
          CGColorSpaceRef cgColorSpace = CGColorSpaceCreateDeviceRGB();
          
          m_offscreenCGImage = CGImageCreate(
            m_windowWidth, m_windowHeight, 8, 32, 4*m_windowWidth,
            cgColorSpace, kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big,
            cgDataProvider, 0, false, kCGRenderingIntentDefault
            );
          CGColorSpaceRelease( cgColorSpace );
          CGDataProviderRelease( cgDataProvider );
          
          FABRIC_VERIFY( aglSetWindowRef( m_windowAGLContext, m_windowRef ) );
        }

        didResize( m_windowWidth, m_windowHeight );
      }
      
      return NPERR_NO_ERROR;
    }
    
    int16_t WindowlessCGViewPort::nppHandleEvent( void *event )
    {
      EventRecord *eventRecord = reinterpret_cast<EventRecord *>( event );
      switch ( eventRecord->what )
      {
        case updateEvt:
        {
          if ( m_cgContext && m_windowRef && m_windowWidth && m_windowHeight )
          {
            RC::Handle<DG::Event> dgRedrawEvent = getRedrawEvent();
            if ( dgRedrawEvent )
            {
              pushOGLContext();
              
              // [pzion 2011022] Fill the viewport with 18% gray adjusted to a
              // gamma of 2.2.  0.46 ~= 0.18^(1/2.2)
              size_t width, height;
              getWindowSize( width, height );
              glViewport( 0, 0, width, height );
              glClearColor( 0.46, 0.46, 0.46, 1.0 );
              glClear( GL_COLOR_BUFFER_BIT );

              try
              {
                dgRedrawEvent->fire();
              }
              catch ( Exception e )
              {
                FABRIC_LOG( "redrawEvent: exception thrown: %s", (const char*)e.getDesc() );
              }
              catch ( ... )
              {
                FABRIC_LOG( "redrawEvent: unknown exception thrown" );
              }
              
              glFinish();
              glReadBuffer( GL_FRONT );
              glReadPixels( 0, 0, m_windowWidth, m_windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, m_offscreenData );

              popOGLContext();

              CGContextSaveGState( m_cgContext );
              CGRect cgRect = CGRectMake( 0, 0, m_windowWidth, m_windowHeight );
              CGContextDrawImage( m_cgContext, cgRect, m_offscreenCGImage );
              CGContextRestoreGState( m_cgContext );
              
              redrawFinished();
            }
          }
        }
        return true;
        
        default:
          return false;
      }
    }

    void WindowlessCGViewPort::pushOGLContext()
    {
      AGLContext prevContext = aglGetCurrentContext();
      if( aglSetCurrentContext( m_windowAGLContext ) == GL_FALSE )
        throw Exception( "Viewport error: unable to set OGL context" );
      m_aglContextStack.push_back( prevContext );
    }
    
    void WindowlessCGViewPort::popOGLContext()
    {
      FABRIC_ASSERT( !m_aglContextStack.empty() );
      AGLContext prevContext = m_aglContextStack.back();
      m_aglContextStack.pop_back();
      if( aglSetCurrentContext( prevContext ) == GL_FALSE )
        throw Exception( "Viewport error: unable to restore previous OGL context" );
    }

    std::string WindowlessCGViewPort::queryUserFilePath( bool existingFile, std::string const &title, std::string const &defaultFilename, std::string const &extension )
    {
      throw Exception( "SaveAs dialog not implemented" );
    }
  };
};
