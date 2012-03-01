/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "WindowsViewPort.h"

#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/Event.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/IO/Dir.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Clients/NPAPI/Context.h>
#include <Fabric/Clients/NPAPI/Interface.h>
#include <algorithm>

#include <windowsx.h>
#include <tchar.h>

#include <GL/GL.h>

namespace Fabric
{
  namespace NPAPI
  {
    RC::Handle<ViewPort> WindowsViewPort::Create( RC::ConstHandle<Interface> const &interface )
    {
      return new WindowsViewPort( interface );
    }
    
    WindowsViewPort::WindowsViewPort( RC::ConstHandle<Interface> const &interface )
      : ViewPort( interface )
      , m_logCollector( interface->getContext()->getLogCollector() )
      , m_npp( interface->getNPP() )
      , m_windowWidth( -1 )
      , m_windowHeight( -1 )
      , m_hWnd( 0 )
      , m_hDC( 0 )
      , m_hGLRC( 0 )
    {
      m_bWindowLess = true;

      uint32_t  bSupportsWindowLess = 0;

      if( NPN_GetValue( m_npp, NPNVSupportsWindowless, (void *)&bSupportsWindowLess ) == NPERR_NO_ERROR )
        bSupportsWindowLess = 0;

      if( m_bWindowLess )
      {
        if( bSupportsWindowLess )
        {
          if ( NPN_SetValue( m_npp, NPPVpluginWindowBool, NULL ) != NPERR_NO_ERROR )
          {
            m_bWindowLess = false;
          }
        }
        else
        {
          m_bWindowLess = false;
        }
      }
    }
    
    WindowsViewPort::~WindowsViewPort()
    {
      if( m_bWindowLess )
      {
      }
      else
      {
        if( m_hWnd ) {
          termOGLContext();
          ::ReleaseDC( m_hWnd, m_hDC );
          m_hWnd = NULL;
        }
      }
    }

    NPError WindowsViewPort::nppDestroy( NPSavedData** save ) {
      if( m_hWnd ) {
        termOGLContext();
        ::ReleaseDC( m_hWnd, m_hDC );
        m_hWnd = NULL;
      }
      return ViewPort::nppDestroy( save );
    }
      
    void WindowsViewPort::needsRedraw()
    {
      if( m_bWindowLess )
      {
        NPRect invalidRect;
        invalidRect.left = m_windowLeft;
        invalidRect.top = m_windowTop;
        invalidRect.right = m_windowLeft+m_windowWidth;
        invalidRect.bottom = m_windowTop+m_windowHeight;
        NPN_InvalidateRect( m_npp, &invalidRect );
      }
      else
      {
        if( m_hWnd )
          ::InvalidateRect( m_hWnd, NULL, FALSE );
      }
    }
    
    NPError WindowsViewPort::nppSetWindow( NPWindow *npWindow )
    {
      bool  needsRedraw = true;

      if( m_bWindowLess )
      {
        if( npWindow->type != NPWindowTypeDrawable )
          return( NPERR_GENERIC_ERROR );

        HDC      hDC = static_cast<HDC>( npWindow->window );
      }
      else
      {
        if( npWindow->type != NPWindowTypeWindow )
          return( NPERR_GENERIC_ERROR );

        HWND    hWnd = (HWND)npWindow->window;
        if( m_hWnd != hWnd )
        {
          if( hWnd )
          {
            ::SetWindowLongPtr( hWnd, GWLP_USERDATA, (LONG_PTR)this );

            // Let's take ownership of the window's message pump.
            m_oldWndProc = SubclassWindow( hWnd, windowsMsgCB );

            m_hDC = ::GetDC( hWnd );

            initOGLContext( );

            // Let's divine the parent window
            m_hParentWnd = ::GetParent( hWnd );

            // If we're running in Chrome multi-process mode, our plugin window
            // is proxied through another window, so we need to go to the grand-parent
            // window to get the one we need to forward mouse and keyboard messages to.
            TCHAR   tcsClassName[ 128 ];
            ::GetClassName( m_hParentWnd, tcsClassName, ARRAYSIZE( tcsClassName ) );
            if( !_tcscmp( tcsClassName, _T( "WrapperNativeWindowClass" ) ) )
            {
              m_hParentWnd = ::GetParent( m_hParentWnd );
            }
          }
          else
          {
            termOGLContext( );
            ::SetWindowLongPtr( hWnd, GWLP_USERDATA, NULL );
            SubclassWindow( m_hWnd, m_oldWndProc );
            m_hParentWnd = NULL;
          }

          m_hWnd = hWnd;
        }
      }

      if( m_hWnd &&
          ( m_windowLeft != npWindow->x || m_windowTop != npWindow->y || 
            m_windowWidth != npWindow->width || m_windowHeight != npWindow->height ) )
      {
        m_windowLeft = npWindow->x;
        m_windowTop = npWindow->y;
        m_windowWidth = npWindow->width;
        m_windowHeight = npWindow->height;

        didResize( m_windowWidth, m_windowHeight );
      }

      return( NPERR_NO_ERROR );
    }
    
    int16_t WindowsViewPort::nppHandleEvent( void *pEvent )
    {
      NPEvent *npEvent = static_cast<NPEvent *>( pEvent );
      
      switch( npEvent->event )
      {
      case WM_PAINT:
        {
            // And this, ladies and gentlemen, is why NPAPI is not 32-bit safe.
          HDC hDC = (HDC)ULongToPtr( npEvent->wParam );
          const RECT *rcPaint = ((const RECT *)ULongToPtr( npEvent->lParam ));

          fireRedrawEvent( );
        }
        break;

      case WM_WINDOWPOSCHANGED:
        break;
      }

      return( 1 );
    }

    LRESULT WindowsViewPort::processMessage( UINT message, WPARAM wParam, LPARAM lParam )
    {
      if( m_hWnd )
      {
        DG::Context::NotificationBracket notificationBracket( getInterface()->getContext() );
        switch( message )
        {
        case WM_PAINT:
        {
          HDC     hDC;
          RECT   rcPaint;
          PAINTSTRUCT     ps;

          ::ZeroMemory( &ps, sizeof( ps ) );
          hDC = BeginPaint( m_hWnd, &ps );
          rcPaint = ps.rcPaint;

          fireRedrawEvent( );
        
          ::glFinish( );
          ::SwapBuffers( m_hDC );

          ::EndPaint( m_hWnd, &ps );
        
          if ( m_logCollector )
            m_logCollector->flush();
        
          redrawFinished();
          return( 0 );
        }

        case WM_ERASEBKGND:
          return( 1 );

        case WM_SETFOCUS:

        case WM_KILLFOCUS:

          break;
        }

        if( message >= WM_MOUSEFIRST && 
            message <= WM_MOUSELAST &&
            message != WM_MOUSEWHEEL /* Chrome will redirect the wheel */ )
        {
          POINT   p = { GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) };
          ::MapWindowPoints( m_hWnd, m_hParentWnd, &p, 1 );

          LPARAM    lParentParam = MAKELONG( p.x, p.y );
          ::PostMessage( m_hParentWnd, message, wParam, lParentParam );
          return( 0 );
        }
        else if( message >= WM_KEYFIRST && message <= WM_KEYLAST )
        {
          ::PostMessage( m_hParentWnd, message, wParam, lParam );
          return( 0 );
        }
      }
      return( DefWindowProc( m_hWnd, message, wParam, lParam ) );
    }

    void WindowsViewPort::fireRedrawEvent( )
    {
      RC::Handle<DG::Event> dgRedrawEvent = getRedrawEvent();
      if ( dgRedrawEvent )
      {
        pushOGLContext();
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
        popOGLContext();
      }
      drawWatermark( m_windowWidth, m_windowHeight );
    }

    void WindowsViewPort::initOGLContext( )
    {
      PIXELFORMATDESCRIPTOR pfd;
      int format;

      ::ZeroMemory( &pfd, sizeof( pfd ) );

      pfd.nSize = sizeof( pfd );
      pfd.nVersion = 1;

      if( m_bWindowLess )
        pfd.dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL | PFD_SUPPORT_GDI;
      else
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

      pfd.iPixelType = PFD_TYPE_RGBA;
      pfd.cColorBits = 24;
      pfd.cDepthBits = 16;
      pfd.iLayerType = PFD_MAIN_PLANE;
      format = ::ChoosePixelFormat( m_hDC, &pfd );
      if( format == 0 )
        throw Exception( "Couldn't get preferred OGL format" );

      if( !::SetPixelFormat( m_hDC, format, &pfd ) )
        throw Exception( "Couldn't set OGL format" );

      m_hGLRC = ::wglCreateContext( m_hDC );

      //Test GL context validity
      pushOGLContext();
      popOGLContext();
    }

    void WindowsViewPort::termOGLContext( )
    {
      ::wglMakeCurrent( 0, 0 );
      ::wglDeleteContext( m_hGLRC );
      m_hGLRC = NULL;
    }

    LRESULT WindowsViewPort::windowsMsgCB( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
    {
      WindowsViewPort *pThis = (WindowsViewPort *)::GetWindowLongPtr( hWnd, GWLP_USERDATA );

      // Ours?
      if( pThis && hWnd == pThis->m_hWnd )
        return( pThis->processMessage( message, wParam, lParam ) );
     
      return( DefWindowProc( hWnd, message, wParam, lParam ) );
    }
      
    void WindowsViewPort::pushOGLContext()
    {
      //WGLDCAndContext prevContext( ::wglGetCurrentDC(), ::wglGetCurrentContext() );
      if ( m_hDC && m_hGLRC )
      {
        if( ::wglMakeCurrent( m_hDC, m_hGLRC ) == FALSE )
          throw Exception( "Viewport error: unable to set OGL context" );
      }
      //m_wglStack.push_back( prevContext );
    }
    
    void WindowsViewPort::popOGLContext()
    {
      //[JeromeCG 20120229] Don't restore previous context (workaround what I think is a Windows OGL threading bug).
      //                    Anyway, each viewport context is set current before anything happens.
      //                    For some reason, in samples with heavy rendering (eg Medical Imaging), setting the previous
      //                    context right after the redraw, even if glFinish() is called, might cause a crash. I suspect
      //                    this might be a OGL thread synchronization issue (on Windows, OGL has a worker thread).
      //                    Note1: I validated that all operators were executed while a valid context was active,
      //                    and we still had that crash for that sample.
      //                    Note2: the crash happens too when prevContext.second != NULL 
      //
      //FABRIC_ASSERT( !m_wglStack.empty() );
      //WGLDCAndContext prevContext( m_wglStack.back() );
      //m_wglStack.pop_back();
      //glFinish();

      //if( ::wglMakeCurrent( prevContext.first, prevContext.second ) == FALSE )
      //  throw Exception( "Viewport error: unable to restore previous OGL context" );
    }

    std::string WindowsViewPort::queryUserFilePath( bool existingFile, std::string const &title, std::string const &defaultFilename, std::string const &extension )
    {
      OPENFILENAME options;
      memset( &options, 0, sizeof(options) );
      options.lStructSize = sizeof(options); 

      options.hwndOwner = m_hWnd;

      char fullPathBuff[1000];
      options.nMaxFile = 1000;
      size_t lenToCopy = std::min( defaultFilename.length(), (size_t)999 );
      strncpy( fullPathBuff, defaultFilename.data(), lenToCopy );
      fullPathBuff[lenToCopy] = 0;
      options.lpstrFile = fullPathBuff;

      std::string extFilter;
      if( !extension.empty() )
      {
        options.lpstrDefExt = extension.c_str();
        extFilter = extension + " file";
        extFilter.push_back('\0');
        extFilter += "*." + extension;
        extFilter.push_back('\0');
        extFilter.push_back('\0');
        options.lpstrFilter = extFilter.c_str();
        options.nFilterIndex = 1;
      }

      if( !title.empty() )
        options.lpstrTitle = title.c_str();

      if( existingFile )
      {
        options.Flags = OFN_FILEMUSTEXIST;
        if( !GetOpenFileName(&options) )
          throw Exception( "Open file failed or was canceled by user" );
      }
      else
      {
        options.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        if( !GetSaveFileName(&options) )
          throw Exception( "Save file failed or was canceled by user" );
      }
      return options.lpstrFile;
    }
  };
};
