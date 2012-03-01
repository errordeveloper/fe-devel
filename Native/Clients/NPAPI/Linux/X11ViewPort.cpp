/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Clients/NPAPI/Linux/X11ViewPort.h>
#include <Fabric/Clients/NPAPI/Interface.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Clients/NPAPI/Context.h>
#include <Fabric/Core/DG/Event.h>
#include <Fabric/Core/MT/LogCollector.h>

namespace Fabric
{
  namespace NPAPI
  {
    RC::Handle<ViewPort> X11ViewPort::Create( RC::ConstHandle<Interface> const &interface )
    {
      NPP npp = interface->getNPP();

      NPBool supportsXEmbed;
      if ( NPN_GetValue( NULL, NPNVSupportsXEmbedBool, (void *)&supportsXEmbed ) != NPERR_NO_ERROR )
        supportsXEmbed = false;

      if ( !supportsXEmbed )
        return 0;

      return new X11ViewPort( interface );
    }
    
    X11ViewPort::X11ViewPort( RC::ConstHandle<Interface> const &interface )
      : ViewPort( interface )
      , m_logCollector( interface->getContext()->getLogCollector() )
      , m_gdkNativeWindow( 0 )
      , m_plug( 0 )
      , m_drawingArea( 0 )
      , m_windowWidth( -1 )
      , m_windowHeight( -1 )
    {
    }
    
    X11ViewPort::~X11ViewPort()
    {
      if ( m_drawingArea )
        g_object_unref( m_drawingArea );
      if ( m_plug )
        g_object_unref( m_plug );
    }
      
    void X11ViewPort::redraw()
    {
      if ( m_drawingArea )
      {
        DG::Context::NotificationBracket notificationBracket( getInterface()->getContext() );
        pushOGLContext();

        static bool printedOpenGLVersionString = false;
        if ( !printedOpenGLVersionString )
        {
          FABRIC_LOG( "OpenGL version string is '%s'", glGetString( GL_VERSION ) );
          printedOpenGLVersionString = true;
        }

        RC::Handle<DG::Event> dgRedrawEvent = getRedrawEvent();
        FABRIC_ASSERT( dgRedrawEvent );
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
        drawWatermark( m_windowWidth, m_windowHeight );
        glFinish();

        GdkGLDrawable *gdkGLDrawable = gdk_gl_drawable_get_current();
        if ( gdk_gl_drawable_is_double_buffered( gdkGLDrawable ) )
          gdk_gl_drawable_swap_buffers( gdkGLDrawable );

        popOGLContext();
        redrawFinished();
      }
    }

    struct ViewPortAndJSONValue
    {
      ViewPort const *viewPort;
      Util::SimpleString json;
    };

    void X11ViewPort::MenuItemActivateCallback( void *_viewPortAndJSONValue )
    {
      ViewPortAndJSONValue const *viewPortAndJSONValue = (ViewPortAndJSONValue const *)_viewPortAndJSONValue;
      viewPortAndJSONValue->viewPort->jsonNotifyPopUpItem( viewPortAndJSONValue->json );
    }

    gboolean X11ViewPort::EventCallback( GtkWidget *widget, GdkEvent *event, gpointer user_data )
    {
      X11ViewPort *x11ViewPort = static_cast< X11ViewPort * >( user_data );
      switch ( event->type )
      {
        case GDK_EXPOSE:
          x11ViewPort->redraw();
          return TRUE;

        case GDK_BUTTON_PRESS:
        {
          GdkEventButton *button = (GdkEventButton *)event;
          if ( button->button == 3 )
          {
            if ( !x11ViewPort->m_popUpItems.empty() )
            {
              GtkWidget *menu = gtk_menu_new();

              for ( PopUpItems::const_iterator it=x11ViewPort->m_popUpItems.begin(); it!=x11ViewPort->m_popUpItems.end(); ++it )
              {
                PopUpItem const &popUpItem = *it;

                GtkWidget *menuItem = gtk_menu_item_new_with_label( popUpItem.desc.c_str() );
                gtk_menu_shell_append( GTK_MENU_SHELL(menu), menuItem );

                ViewPortAndJSONValue *viewPortAndJSONValue = new ViewPortAndJSONValue;
                viewPortAndJSONValue->viewPort = x11ViewPort;
                viewPortAndJSONValue->json = popUpItem.argJSON;
                g_signal_connect_swapped( G_OBJECT(menuItem), "activate",
                  G_CALLBACK(&X11ViewPort::MenuItemActivateCallback),
                  viewPortAndJSONValue );
                gtk_widget_show( menuItem );
              }

              gtk_menu_popup( GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time() );

              return TRUE;
            }
          }
        }    
        break;
      }
      return FALSE;
    }
    
    NPError X11ViewPort::nppSetWindow( NPWindow *window )
    {
      static bool calledGtkGLInit = false;
      if ( !calledGtkGLInit )
      {
        int argc = 1;
        char **argv;
        argv = new char *[2];
        argv[0] = strdup("libFabricNPAPIPlugin.so");
        argv[1] = NULL;
        gtk_gl_init( &argc, &argv );
        gdk_gl_init( &argc, &argv );
        calledGtkGLInit = true;
      }

      GdkNativeWindow gdkNativeWindow = (GdkNativeWindow)(size_t)window->window;

      if ( gdkNativeWindow != m_gdkNativeWindow )
      {
        if ( m_drawingArea )
        {
          g_object_unref( m_drawingArea );
          m_drawingArea = 0;
        }

        if ( m_plug )
        {
          g_object_unref( m_plug );
          m_plug = 0;
        }

        m_gdkNativeWindow = gdkNativeWindow;

        if ( gdkNativeWindow )
        {
          GdkGLConfig *gdkGLConfig = gdk_gl_config_new_by_mode( GdkGLConfigMode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE) );
          FABRIC_ASSERT( gdkGLConfig );


          m_drawingArea = gtk_drawing_area_new();
          FABRIC_ASSERT( m_drawingArea );
          g_object_ref( m_drawingArea );
          bool gtkWidgetSetGLCapabilityResult = gtk_widget_set_gl_capability( m_drawingArea, gdkGLConfig, NULL, true, GDK_GL_RGBA_TYPE );
          FABRIC_ASSERT( gtkWidgetSetGLCapabilityResult );
          gtk_widget_set_events( m_drawingArea,
            GDK_EXPOSURE_MASK
            /* disabled the button press masks, since javascript
               events stopped to work
            | GDK_BUTTON_PRESS_MASK
            | GDK_BUTTON_RELEASE_MASK
              */
            );
          
          m_plug = gtk_plug_new( gdkNativeWindow );
          FABRIC_ASSERT( m_plug );
          g_object_ref( m_plug );
          gtk_container_add( GTK_CONTAINER(m_plug), m_drawingArea ); 

          gtk_widget_show_all( GTK_WIDGET(m_plug) );

          g_signal_connect( G_OBJECT(m_drawingArea), "event", G_CALLBACK( &X11ViewPort::EventCallback ), this );

          //Test GL context validity
          pushOGLContext();
          popOGLContext();
        }
      }

      if ( m_windowLeft != window->x || m_windowTop != window->y
        || m_windowWidth != window->width || m_windowHeight != window->height )
      {
        m_windowLeft = window->x;
        m_windowTop = window->y;
        m_windowWidth = window->width;
        m_windowHeight = window->height;
      
        didResize( m_windowWidth, m_windowHeight );
      }

      return NPERR_NO_ERROR;
    }
    
    NPError X11ViewPort::nppGetValue( NPPVariable variable, void *value )
    {
      if ( variable == NPPVpluginNeedsXEmbed )
      {
        *(static_cast<NPBool*>(value)) = true;
        return NPERR_NO_ERROR;
      }

      return ViewPort::nppGetValue( variable, value );
    }

    void X11ViewPort::needsRedraw()
    {
      gdk_window_invalidate_rect(
        gtk_widget_get_window( m_drawingArea ),
        NULL,
        false
        );
    }
      
    void X11ViewPort::pushOGLContext()
    {
      GdkGLDrawableAndContext prevContext( gdk_gl_drawable_get_current(), gdk_gl_context_get_current() );

      if ( m_drawingArea )
      {
        GdkGLDrawable *gdkGLDrawable = gtk_widget_get_gl_drawable( m_drawingArea );
        FABRIC_ASSERT( gdkGLDrawable );
        GdkGLContext *gdkGLContext = gtk_widget_get_gl_context( m_drawingArea );
        FABRIC_ASSERT( gdkGLContext );

        if( !gdk_gl_drawable_make_current( gdkGLDrawable, gdkGLContext ) )
          throw Exception( "Viewport error: unable to set OGL context" );
      }
      m_gdkGLStack.push_back( prevContext );
    }
    
    void X11ViewPort::popOGLContext()
    {
      FABRIC_ASSERT( !m_gdkGLStack.empty() );
      GdkGLDrawableAndContext prevContext( m_gdkGLStack.back() );
      m_gdkGLStack.pop_back();

      if( prevContext.first && !gdk_gl_drawable_make_current( prevContext.first, prevContext.second ) )
        throw Exception( "Viewport error: unable to restore previous OGL context" );
    }

    std::string X11ViewPort::queryUserFilePath( bool existingFile, std::string const &title, std::string const &defaultFilename, std::string const &extension )
    {
      GtkWidget *dialog = gtk_file_chooser_dialog_new( title.c_str(),
        NULL,
        existingFile ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        existingFile ? GTK_STOCK_OPEN : GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL
        );
      gtk_file_chooser_set_do_overwrite_confirmation( GTK_FILE_CHOOSER(dialog), TRUE );
      //gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER(dialog), default_folder_for_saving );
      if ( !existingFile )
        gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER(dialog), defaultFilename.c_str() );
       
      std::string result;
      gint runResult = gtk_dialog_run(GTK_DIALOG(dialog));
      if ( runResult == GTK_RESPONSE_ACCEPT )
      {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        result = std::string( filename );
        g_free(filename);
      }
      gtk_widget_destroy (dialog);
      if ( runResult != GTK_RESPONSE_ACCEPT )
      {
        throw Exception( existingFile ? "Open file failed or was canceled by user" : "Save file failed or was canceled by user" );
      }
      return result;
    }
  };
};
