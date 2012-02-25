#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import os
import sys
import json
import ctypes
import collections
import atexit
import Queue
import signal

# FIXME Windows
if os.name == 'posix':
  _fabric = ctypes.CDLL( os.path.dirname( __file__ ) + '/libFabricPython.so' )
else:
  raise Exception('not implemented for Windows yet!')

# FIXME Windows
_caughtSIGINT = False
def _handleSIGINT( signum, frame ):
  global _caughtSIGINT
  _caughtSIGINT = True
signal.signal( signal.SIGINT, _handleSIGINT )

# catch uncaught exceptions so that we don't wait on threads 
_uncaughtException = False
_oldExceptHook = sys.excepthook
def _excepthook( type, value, traceback):
  global _uncaughtException
  _uncaughtException = True
  _oldExceptHook( type, value, traceback )
sys.excepthook = _excepthook

# print app and version information
_fabric.identify()

def createClient():
  return _INTERFACE( _fabric )

# used in unit tests
def stringify( obj ):
  return json.dumps( _normalizeForUnitTests( _typeToDict( obj ) ) )

# global for tracking GC ids for core objects
_gcId = 0
def _getNextGCId():
  global _gcId
  _gcId = _gcId + 1
  return _gcId

# for unit tests only, make floats use same precision across different
# versions of python which have different repr() implementations and
# change dicts to sorted lists so ordering doesn't change
def _normalizeForUnitTests( obj ):
  if type( obj ) is list:
    objlist = []
    for elem in obj:
      objlist.append( _normalizeForUnitTests( elem ) )
    return objlist
  elif type( obj ) is dict:
    objdictlist = []
    for member in obj:
      elemobj = {}
      elemobj[ member ] = _normalizeForUnitTests( obj[ member ] )
      objdictlist.append( elemobj )
    objdictlist.sort()
    return objdictlist
  elif type( obj ) is float:
    return format( obj, '.3f' )
  else:
    return obj

# take a python class and convert its members down to a hierarchy of
# dictionaries, ignoring methods
def _typeToDict( obj ):
  if type( obj ) is list:
    objlist = []
    for elem in obj:
      objlist.append( _typeToDict( elem ) )
    return objlist

  elif not hasattr( obj, '__dict__' ):
    return obj

  else:
    objdict = {}
    for member in vars( obj ):
      attr = getattr( obj, member )
      objdict[ member ] = _typeToDict( attr )
    return objdict

# this is the interface object that gets returned to the user
class _INTERFACE( object ):
  def __init__( self, fabric ):
    self.__client = _CLIENT( fabric )
    self.KLC = self.__client.klc
    self.MR = self.__client.mr
    self.RT = self.__client.rt
    self.RegisteredTypesManager = self.RT
    self.DG = self.__client.dg
    self.DependencyGraph = self.DG
    self.VP = self.__client.vp
    self.EX = self.__client.ex
    self.IO = self.__client.io
    self.build = self.__client.build

  def flush( self ):
    self.__client.executeQueuedCommands()

  def close( self ):
    self.__client.close()

  def running( self ):
    return self.__client.running()

  def getMemoryUsage( self ):
    # dictionary hack to simulate Python 3.x nonlocal
    memoryUsage = { '_': None }
    def __getMemoryUsage( result ):
      memoryUsage[ '_' ] = result

    self.__client.queueCommand( [], 'getMemoryUsage', None, None, __getMemoryUsage )
    self.flush()

    return memoryUsage[ '_' ]

class _CLIENT( object ):
  def __init__( self, fabric ):
    self.__fabric = fabric
    self.__fabricClient = self.__createClient()

    self.__queuedCommands = []
    self.__queuedUnwinds = []
    self.__queuedCallbacks = []

    self.gc = _GC( self )
    self.klc = _KLC( self )
    self.mr = _MR( self )
    self.rt = _RT( self )
    self.dg = _DG( self )
    self.vp = _VP( self )
    self.ex = _EX( self )
    self.io = _IO( self )
    self.build = _BUILD( self )

    self.__closed = False
    self.__state = {}

    self.__notifications = Queue.Queue()

    # declare all class variables needed in the notifyCallback above
    # here as the closure remembers the current class members immediately
    self.__NOTIFYCALLBACK = ctypes.CFUNCTYPE( None, ctypes.c_char_p )
    self.__registerNotifyCallback()
    self.__processAllNotifications()

    # prevent exit until all our threads complete
    atexit.register( self.__waitForClose )

  def running( self ):
    self.__processAllNotifications()
    return not self.__closed

  def __processAllNotifications( self ):
    while not self.__notifications.empty():
      self.__processOneNotification()

  def __processOneNotification( self, timeout = None ):
    n = None
    try:
      n = self.__notifications.get( True, timeout )
    except Queue.Empty:
      return

    arg = None
    if 'arg' in n:
      arg = n[ 'arg' ]
    self._route( n[ 'src' ], n[ 'cmd' ], arg )
    n = self.__notifications.task_done()

  def __runScheduledCallbacks( self ):
    self.__fabric.runScheduledCallbacks( self.__fabricClient )

  def __waitForClose( self ):
    if not _uncaughtException:
      while not _caughtSIGINT and (
          not self.__closed or not self.__notifications.empty()
        ):
        # FIXME only using timeout so we can allow a Ctrl-C after
        # trying to exit without correctly using client.close()
        self.__processOneNotification( 0.1 )

  def __createClient( self ):
    result = ctypes.c_void_p()
    self.__fabric.createClient( ctypes.pointer( result ) )
    return result

  def __jsonExec( self, data, length ):
    result = ctypes.c_char_p()

    if self.__closed:
      raise Exception( 'Fabric client has already been closed' )

    self.__fabric.jsonExec(
      self.__fabricClient,
      data,
      length,
      ctypes.pointer( result )
    )

    return result

  def close( self ):
    self.__closed = True
    self.__fabric.freeClient( self.__fabricClient )

  def getLicenses( self ):
    return self.__state.licenses;

  def getContextID( self ):
    return self.__state.contextID;

  def queueCommand( self, dst, cmd, arg = None, unwind = None, callback = None ):
    command = { 'dst': dst, 'cmd': cmd }
    if ( arg is not None ):
      command[ 'arg' ] = arg

    self.__queuedCommands.append( command )
    self.__queuedUnwinds.append( unwind )
    self.__queuedCallbacks.append( callback )

  def executeQueuedCommands( self ):
    commands = self.__queuedCommands
    self.__queuedCommands = []
    unwinds = self.__queuedUnwinds
    self.__queuedUnwinds = []
    callbacks = self.__queuedCallbacks
    self.__queuedCallbacks = []

    if len( commands ) < 1:
      return

    jsonEncodedCommands = json.dumps( commands )
    jsonEncodedResults = self.__jsonExec( jsonEncodedCommands, len( jsonEncodedCommands ) )

    try:
      results = json.loads( jsonEncodedResults.value )
    except Exception:
      raise Exception( 'unable to parse JSON results: ' + jsonEncodedResults )
    self.__fabric.freeString( self.__fabricClient, jsonEncodedResults )

    for i in range(len(results)):
      result = results[i]
      callback = callbacks[i]

      if ( 'exception' in result ):
        for j in range( len( unwinds ) - 1, i, -1 ):
          unwind = unwinds[ j ]
          if ( unwind is not None ):
            unwind()
        self.__processAllNotifications()
        raise Exception( 'Fabric core exception: ' + result[ 'exception' ] )
      elif ( callback is not None ):
        callback( result[ 'result' ] )

    self.__processAllNotifications()

  def _handleStateNotification( self, newState ):
    self.__state = {}
    self._patch( newState )

    if 'build' in newState:
      self.build._handleStateNotification( newState[ 'build' ] )
    self.dg._handleStateNotification( newState[ 'DG' ] )
    self.rt._handleStateNotification( newState[ 'RT' ] )
    self.ex._handleStateNotification( newState[ 'EX' ] )
    if 'VP' in newState:
      self.vp.handleStateNotification( newState[ 'VP' ] )

  def _patch( self, diff ):
    if 'licenses' in diff:
      self.__state[ 'licenses' ] = diff[ 'licenses' ]
    if 'contextID' in diff:
      self.__state[ 'contextID' ] = diff[ 'contextID' ]

  def _handle( self, cmd, arg ):
    try:
      if cmd == 'state':
        self._handleStateNotification( arg )
      else:
        raise Exception( 'unknown command' )
    except Exception as e:
      raise Exception( 'command "' + cmd + '": ' + str( e ) )

  def _route( self, src, cmd, arg ):
    if len(src) == 0:
      self._handle( cmd, arg )
    else:
      src = collections.deque( src )
      firstSrc = src.popleft()

      if firstSrc == 'RT':
        self.rt._route( src, cmd, arg )
      elif firstSrc == 'DG':
        self.dg._route( src, cmd, arg )
      elif firstSrc == 'EX':
        self.ex._route( src, cmd, arg )
      elif firstSrc == 'VP':
        self.vp._route( src, cmd, arg )
      elif firstSrc == 'GC':
        self.gc._route( src, cmd, arg )
      elif firstSrc == 'ClientWrap':
        if cmd == 'runScheduledCallbacks':
          self.__runScheduledCallbacks()
        else:
          raise Exception( 'bad ClientWrap cmd: "' + cmd + '"' )
      else:
        raise Exception( 'unroutable src: ' + firstSrc )

  def __notifyCallback( self, jsonEncodedNotifications ):
    try:
      notifications = json.loads( jsonEncodedNotifications )
    except Exception:
      raise Exception( 'unable to parse JSON notifications' )

    for i in range( 0, len( notifications ) ):
      self.__notifications.put( notifications[i] )

  def __getNotifyCallback( self ):
    # use a closure here so that 'self' is maintained without us
    # explicitly passing it
    def notifyCallback( jsonEncodedNotifications ):
      self.__notifyCallback( jsonEncodedNotifications )

    # this is important, we have to maintain a reference to the CFUNCTYPE
    # ptr and not just return it, otherwise it will be garbage collected
    # and callbacks will fail
    self.__CFUNCTYPE_notifyCallback = self.__NOTIFYCALLBACK ( notifyCallback )
    return self.__CFUNCTYPE_notifyCallback

  def __registerNotifyCallback( self ):
    self.__fabric.setJSONNotifyCallback( self.__fabricClient, self.__getNotifyCallback() )

class _GCOBJECT( object ):
  def __init__( self, nsobj ):
    self.__id = "GC_" + str( _getNextGCId() )
    self.__nextCallbackID = 0
    self.__callbacks = {}
    self._nsobj = nsobj
    nsobj._getClient().gc.addObject( self )

  def __del__( self ):
    self.__dispose()

  def __dispose( self ):
    self._gcObjQueueCommand( 'dispose' )
    self.__nsobj._getClient().gc.disposeObject( self )
    self.__id = None
 
  def _gcObjQueueCommand( self, cmd, arg = None, unwind = None, callback = None ):
    if self.__id is None:
      raise Exception( "GC object has already been disposed" )
    self._nsobj._objQueueCommand( [ self.__id ], cmd, arg, unwind, callback )

  def _synchronousGetOnly( self, cmd ):
    # dictionary hack to simulate Python 3.x nonlocal
    data = { '_': None }
    def __callback( result ):
      data[ '_' ] = result
    self._gcObjQueueCommand( cmd, None, None, __callback )
    self._nsobj._executeQueuedCommands()
    return data[ '_' ]

  def _registerCallback( self, callback ):
    self.__nextCallbackID = self.__nextCallbackID + 1
    callbackID = self.__nextCallbackID
    self.__callbacks[ callbackID ] = callback
    return callbackID

  def _route( self, src, cmd, arg ):
    callback = self.__callbacks[ arg[ 'serial' ] ]
    del self.__callbacks[ arg[ 'serial' ] ]
    callback( arg[ 'result' ] )

  def getID( self ):
    return self.__id

  def setID( self, id ):
    self.__id = id

  def unwind( self ):
    self.setID( None )

class _NAMESPACE( object ):
  def __init__( self, client, name ):
    self.__client = client
    self.__name = name

  def _getClient( self ):
    return self.__client

  def _getName( self ):
    return self.__namespace

  def _objQueueCommand( self, dst, cmd, arg = None, unwind = None, callback = None ):
    if dst is not None:
      dst = [ self.__name ] + dst
    else:
      dst = [ self.__name ]
    self.__client.queueCommand( dst, cmd, arg, unwind, callback )

  def _queueCommand( self, cmd, arg = None, unwind = None, callback = None ):
    self._objQueueCommand( None, cmd, arg, unwind, callback )

  def _executeQueuedCommands( self ):
    self.__client.executeQueuedCommands()

class _DG( _NAMESPACE ):
  def __init__( self, client ):
    super( _DG, self ).__init__( client, 'DG' )
    self._namedObjects = {}

  def createBinding( self ):
    return self._BINDING()

  def _createBindingList( self, dst ):
    return self._BINDINGLIST( self, dst )

  def __createNamedObject( self, name, cmd, objType ):
    if name in self._namedObjects:
      raise Exception( 'a NamedObject named "' + name + '" already exists' )
    obj = objType( self, name )
    self._namedObjects[ name ] = obj
    
    def __unwind():
      obj._destroy()

    self._queueCommand( cmd, name, __unwind )

    return obj

  def createOperator( self, name ):
    return self.__createNamedObject( name, 'createOperator', self._OPERATOR )
   
  def createNode( self, name ):
    return self.__createNamedObject( name, 'createNode', self._NODE )

  def createResourceLoadNode( self, name ):
    return self.__createNamedObject( name, 'createResourceLoadNode', self._RESOURCELOADNODE )

  def createEvent( self, name ):
    return self.__createNamedObject( name, 'createEvent', self._EVENT )

  def createEventHandler( self, name ):
    return self.__createNamedObject( name, 'createEventHandler', self._EVENTHANDLER )

  def getAllNamedObjects( self ):
    result ={}
    for namedObjectName in self._namedObjects:
      result[ namedObjectName ] = self._namedObjects[ namedObjectName ]
    return result

  def __getOrCreateNamedObject( self, name, type ):
    if name not in self._namedObjects:
      if type == 'Operator':
        self.createOperator( name )
      elif type == 'Node':
        self.createNode( name )
      elif type == 'Event':
        self.createEvent( name )
      elif type == 'EventHandler':
        self.createEventHandler( name )
      else:
        raise Exception( 'unhandled type "' + type + '"' )
    return self._namedObjects[ name ]

  def _handleStateNotification( self, state ):
    self._namedObjects = {}
    for namedObjectName in state:
      namedObjectState = state[ namedObjectName ]
      self.__getOrCreateNamedObject( namedObjectName, namedObjectState[ 'type' ] )
    for namedObjectName in state:
      self._namedObjects[ namedObjectName ]._patch( state[ namedObjectName ] )

  def _handle( self, cmd, arg ):
    # FIXME no logging callback implemented yet
    if cmd == 'log':
      pass
      #if ( self.__logCallback ):
      #  self.__logCallback( arg )
    else:
      raise Exception( 'command "' + cmd + '": unrecognized' )

  def _route( self, src, cmd, arg ):
    if len( src ) == 0:
      self._handle( cmd, arg )
    else:
      src = collections.deque( src )
      namedObjectName = src.popleft()
      namedObjectType = None
      if type( arg ) is dict and 'type' in arg:
        namedObjectType = arg[ 'type' ]
      self.__getOrCreateNamedObject( namedObjectName, namedObjectType )._route( src, cmd, arg )

  class _NAMEDOBJECT( object ):
    def __init__( self, dg, name ):
      self.__name = name
      self.__errors = []
      self._dg = dg
  
    def _nObjQueueCommand( self, cmd, arg = None, unwind = None, callback = None ):
      if self.__name is None:
        raise Exception( 'NamedObject "' + name + '" has been deleted' )
      self._dg._objQueueCommand( [ self.__name ], cmd, arg, unwind, callback )
  
    def _patch( self, diff ):
      if 'errors' in diff:
        self.__errors = diff[ 'errors' ]
  
    def _destroy( self ):
      del self._dg._namedObjects[ self.__name ]
      self.__name = None
  
    def _handle( self, cmd, arg ):
      if cmd == 'delta':
        self._patch( arg )
      else:
        raise Exception( 'command "' + cmd + '" not recognized' )
  
    def _route( self, src, cmd, arg ):
      if len( src ) == 0:
        self._handle( cmd, arg )
      else:
        raise Exception( 'unroutable' )
  
    def getName( self ):
      return self.__name
  
    def getErrors( self ):
      self._dg._executeQueuedCommands()
      return self.__errors
  
  class _BINDINGLIST( object ):
    def __init__( self, dg, dst ):
      self.__bindings = []
      self._dg = dg
      self.__dst = dst

    def _patch( self, state ):
      self.__bindings = []
      for i in range( 0, len( state ) ):
        binding = {
          'operator': self._dg._namedObjects[ state[ i ][ 'operator' ] ],
          'parameterLayout': state[ i ][ 'parameterLayout' ]
        }
        self.__bindings.append( binding )

    def _handle( self, cmd, arg ):
      if cmd == 'delta':
        self._patch( arg )
      else:
        raise Exception( 'command "' + cmd + '": unrecognized' )

    def _route( self, src, cmd, arg ):
      if len( src ) == 0:
        self._handle( cmd, arg )
      else:
        raise Exception( 'unroutable' )

    def _handleStateNotification( self, state ):
      self._patch( state )
   
    def empty( self ):
      if self.__bindings is None:
        self._dg._executeQueuedCommands()
      return len( self.__bindings ) == 0
  
    def getLength( self ):
      if self.__bindings is None:
        self._dg._executeQueuedCommands()
      return len( self.__bindings )
  
    def getOperator( self, index ):
      if self.__bindings is None:
        self._dg._executeQueuedCommands()
      return self.__bindings[ index ].getOperator()
      
    def append( self, binding ):
      operatorName = None
      try:
        operatorName = binding.getOperator().getName()
      except Exception:
        raise 'operator: not an operator'
  
      oldBindings = self.__bindings
      self.__bindings = None
  
      def __unwind():
        self.__bindings = oldBindings
      args = {
        'operatorName': operatorName,
        'parameterLayout': binding.getParameterLayout()
      }
      self._dg._objQueueCommand( self.__dst, 'append', args, __unwind )
  
    def insert( self, binding, beforeIndex ):
      operatorName = None
      try:
        operatorName = binding.getOperator().getName()
      except Exception:
        raise 'operator: not an operator'
  
      if type( beforeIndex ) is not int:
        raise Exception( 'beforeIndex: must be an integer' )
  
      oldBindings = self.__bindings
      self.__bindings = None
      
      def __unwind():
        self.__bindings = oldBindings
      args = {
        'beforeIndex': beforeIndex,
        'operatorName': operatorName,
        'parameterLayout': binding.getParameterLayout()
      }
      self._dg._objQueueCommand( self.__dst, 'insert', args, __unwind )
  
    def remove( self, index ):
      oldBindings = self.__bindings
      self.__bindings = None
      
      def __unwind():
        self.__bindings = oldBindings
      args = {
        'index': index,
      }
      self._dg._objQueueCommand( self.__dst, 'remove', args, __unwind )
  
  class _BINDING( object ):
    def __init__( self ):
      self.__operator = None
      self.__parameterLayout = None
  
    def getOperator( self ):
      return self.__operator
  
    def setOperator( self, operator ):
      self.__operator = operator
  
    def getParameterLayout( self ):
      return self.__parameterLayout
  
    def setParameterLayout( self, parameterLayout ):
      self.__parameterLayout = parameterLayout
  
  class _OPERATOR( _NAMEDOBJECT ):
    def __init__( self, dg, name ):
      super( _DG._OPERATOR, self ).__init__( dg, name )
      self.__diagnostics = []
      self.__filename = None
      self.__sourceCode = None
      self.__entryFunctionName = None
      self.__mainThreadOnly = None

    def _patch( self, diff ):
      super( _DG._OPERATOR, self )._patch( diff )

      if 'filename' in diff:
        self.__filename = diff[ 'filename' ]

      if 'sourceCode' in diff:
        self.__sourceCode = diff[ 'sourceCode' ]

      if 'entryFunctionName' in diff:
        self.__entryFunctionName = diff[ 'entryFunctionName' ]

      if 'diagnostics' in diff:
        self.__diagnostics = diff[ 'diagnostics' ]

      if 'mainThreadOnly' in diff:
        self.__mainThreadOnly = diff[ 'mainThreadOnly' ]

    def getMainThreadOnly( self ):
      if self.__mainThreadOnly is None:
        self._dg._executeQueuedCommands()
      return self.__mainThreadOnly

    def setMainThreadOnly( self, mainThreadOnly ):
      oldMainThreadOnly = self.__mainThreadOnly
      self.__mainThreadOnly = mainThreadOnly

      def __unwind():
        self.__mainThreadOnly = oldMainThreadOnly

      self._nObjQueueCommand( 'setMainThreadOnly', mainThreadOnly, __unwind )

    def getFilename( self ):
      if self.__filename is None:
        self._dg._executeQueuedCommands()
      return self.__filename

    def getSourceCode( self ):
      if self.__sourceCode is None:
        self._dg._executeQueuedCommands()
      return self.__sourceCode

    def setSourceCode( self, filename, sourceCode = None ):
      # this is legacy usage, sourceCode only
      if sourceCode is None:
        sourceCode = filename
        filename = "(unknown)"

      oldFilename = self.__filename
      self.__filename = filename
      oldSourceCode = self.__sourceCode
      self.__sourceCode = sourceCode
      oldDiagnostics = self.__diagnostics
      self.__diagnostics = []

      def __unwind():
        self.__filename = oldFilename
        self.__sourceCode = oldSourceCode
        self.__diagnostics = oldDiagnostics

      args = {
        'filename': filename,
        'sourceCode': sourceCode
      }
      self._nObjQueueCommand( 'setSourceCode', args, __unwind )

    def getEntryFunctionName( self ):
      if self.__entryFunctionName is None:
        self._dg._executeQueuedCommands()
      return self.__entryFunctionName

    def setEntryFunctionName( self, entryFunctionName ):
      oldEntryFunctionName = self.__entryFunctionName
      self.__entryFunctionName = entryFunctionName

      def __unwind():
        self.__entryFunctionName = oldEntryFunctionName

      self._nObjQueueCommand( 'setEntryFunctionName', entryFunctionName, __unwind )
      self.__diagnostics = []

    def getDiagnostics( self ):
      if len( self.__diagnostics ) == 0:
        self._dg._executeQueuedCommands()
      return self.__diagnostics

  class _CONTAINER( _NAMEDOBJECT ):
    def __init__( self, dg, name ):
      super( _DG._CONTAINER, self ).__init__( dg, name )
      self.__rt = dg._getClient().rt
      self.__members = None
      self.__size = None
      self.__sizeNeedRefresh = True

    def _patch( self, diff ):
      super( _DG._CONTAINER, self )._patch( diff )

      if 'members' in diff:
        self.__members = diff[ 'members' ]

      if 'size' in diff:
        self.__size = diff[ 'size' ]

    def _handle( self, cmd, arg ):
      if cmd == 'dataChange':
        memberName = arg[ 'memberName' ]
        sliceIndex = arg[ 'sliceIndex' ]
        # FIXME invalidate cache here, see pzion comment in node.js
      else:
        super( _DG._CONTAINER, self )._handle( cmd, arg )

    def getCount( self ):
      if self.__sizeNeedRefresh:
        self.__sizeNeedRefresh = None
        self._dg._executeQueuedCommands()
      return self.__size

    def size( self ):
      return self.getCount()

    def setCount( self, count ):
      self._nObjQueueCommand( 'resize', count )
      self.__sizeNeedRefresh = True

    def resize( self, count ):
      self.setCount( count )

    def getMembers( self ):
      if self.__members is None:
        self._dg._executeQueuedCommands()
      return self.__members

    def addMember( self, memberName, memberType, defaultValue = None ):
      if self.__members is None:
        self.__members = {}
      if memberName in self.__members:
        raise Exception( 'there is already a member named "' + memberName + '"' )

      arg = { 'name': memberName, 'type': memberType }
      if defaultValue is not None:
        arg[ 'defaultValue' ] = defaultValue

      self.__members[ memberName ] = arg

      def __unwind():
        del self.__members[ memberName ]

      self._nObjQueueCommand( 'addMember', arg, __unwind )

    def removeMember( self, memberName ):
      if self.__members is None or memberName not in self.__members:
        raise Exception( 'there is no member named "' + memberName + '"' )

      oldMember = self.__members[ memberName ]
      del self.__members[ memberName ]

      def __unwind():
        self.__members[ memberName ] = oldMember

      self._nObjQueueCommand( 'removeMember', memberName, __unwind )

    def getData( self, memberName, sliceIndex = None ):
      if sliceIndex is None:
        sliceIndex = 0

      # dictionary hack to simulate Python 3.x nonlocal
      data = { '_': None }
      def __callback( result ):
        data[ '_' ] = self.__rt._assignPrototypes( result,
          self.__members[ memberName ][ 'type' ] )

      args = { 'memberName': memberName, 'sliceIndex': sliceIndex }
      self._nObjQueueCommand( 'getData', args, None, __callback )
      self._dg._executeQueuedCommands()
      return data[ '_' ]

    def getDataJSON( self, memberName, sliceIndex = None ):
      if sliceIndex is None:
        sliceIndex = 0

      # dictionary hack to simulate Python 3.x nonlocal
      data = { '_': None }
      def __callback( result ):
        data[ '_' ] = result

      args = { 'memberName': memberName, 'sliceIndex': sliceIndex }
      self._nObjQueueCommand( 'getDataJSON', args, None, __callback )
      self._dg._executeQueuedCommands()
      return data[ '_' ]

    def getDataSize( self, memberName, sliceIndex ):
      # dictionary hack to simulate Python 3.x nonlocal
      data = { '_': None }
      def __callback( result ):
        data[ '_' ] = result

      args = { 'memberName': memberName, 'sliceIndex': sliceIndex }
      self._nObjQueueCommand( 'getDataSize', args, None, __callback )
      self._dg._executeQueuedCommands()
      return data[ '_' ]
      
    def getDataElement( self, memberName, sliceIndex, elementIndex ):
      # dictionary hack to simulate Python 3.x nonlocal
      data = { '_': None }
      def __callback( result ):
        data[ '_' ] = self.__rt._assignPrototypes(
          result,
          # remove the braces since we are getting a single element
          self.__members[ memberName ][ 'type' ][0:-2]
        )

      args = {
        'memberName': memberName,
        'sliceIndex': sliceIndex,
        'elementIndex': elementIndex
      }
      self._nObjQueueCommand( 'getDataElement', args, None, __callback )
      self._dg._executeQueuedCommands()
      return data[ '_' ]

    def setData( self, memberName, sliceIndex, data = None ):
      if data is None:
        data = sliceIndex
        sliceIndex = 0

      args = {
        'memberName': memberName,
        'sliceIndex': sliceIndex,
        'data': _typeToDict( data )
      }
      self._nObjQueueCommand( 'setData', args )

    def getBulkData( self ):
      # dictionary hack to simulate Python 3.x nonlocal
      data = { '_': None }
      def __callback( result ):
        for memberName in result:
          member = result[ memberName ]
          for i in range( 0, len( member ) ):
            # FIXME this is incorrect, ignoring return value
            self.__rt._assignPrototypes(
              member[ i ],
              self.__members[ memberName ][ 'type' ]
            )
        data[ '_' ] = result

      self._nObjQueueCommand( 'getBulkData', None, None, __callback )
      self._dg._executeQueuedCommands()
      return data[ '_' ]

    def setBulkData( self, data ):
      self._nObjQueueCommand( 'setBulkData', data )

    def getSliceBulkData( self, index ):
      if type( index ) is not int:
        raise Exception( 'index: must be an integer' )
      return self.getSlicesBulkData( [ index ] )[ 0 ]

    def getSlicesBulkData( self, indices ):
      # dictionary hack to simulate Python 3.x nonlocal
      data = { '_': None }
      def __callback( result ):
        obj = []
        for i in range( 0, len( result ) ):
          sliceobj = {}
          obj.append( sliceobj )
          for memberName in result[ i ]:
            sliceobj[ memberName ] = self.__rt._assignPrototypes(
              result[ i ][ memberName ],
              self.__members[ memberName ][ 'type' ]
            )
        data[ '_' ] = obj

      self._nObjQueueCommand( 'getSlicesBulkData', indices, None, __callback )
      self._dg._executeQueuedCommands()
      return data[ '_' ]

    def getMemberBulkData( self, member ):
      if type( member ) is not str:
        raise Exception( 'member: must be a string' )
      return self.getMembersBulkData( [ member ] )[ member ]
     
    def getMembersBulkData( self, members ):
      # dictionary hack to simulate Python 3.x nonlocal
      data = { '_': None }
      def __callback( result ):
        obj = {}
        for member in result:
          memberobj = []
          obj[ member ] = memberobj

          memberData = result[ member ]
          for i in range( 0, len( memberData ) ):
            memberobj.append( self.__rt._assignPrototypes(
              memberData[ i ],
              self.__members[ member ][ 'type' ]
              )
            )
        data[ '_' ] = obj

      self._nObjQueueCommand( 'getMembersBulkData', members, None, __callback )
      self._dg._executeQueuedCommands()
      return data[ '_' ]

    def setSlicesBulkData( self, data ):
      self._nObjQueueCommand( 'setSlicesBulkData', data )

    def setSliceBulkData( self, sliceIndex, data ):
      args = [ { 'sliceIndex': sliceIndex, 'data': data } ]
      self._nObjQueueCommand( 'setSlicesBulkData', args )

    def getBulkDataJSON( self ):
      # dictionary hack to simulate Python 3.x nonlocal
      data = { '_': None }
      def __callback( result ):
        data[ '_' ] = result

      self._nObjQueueCommand( 'getBulkDataJSON', None, None, __callback )
      self._dg._executeQueuedCommands()
      return data[ '_' ]

    def setBulkDataJSON( self, data ):
      self._nObjQueueCommand( 'setBulkDataJSON', data )
     
    def putResourceToFile( self, fileHandle, memberName ):
      args = {
        'memberName': memberName,
        'file': fileHandle
      }
      self._nObjQueueCommand( 'putResourceToFile', args )
      self._dg._executeQueuedCommands()
      
  class _NODE( _CONTAINER ):
    def __init__( self, dg, name ):
      super( _DG._NODE, self ).__init__( dg, name )
      self.__dependencies = {}
      self.__evaluateAsyncFinishedSerial = 0
      self.__evaluateAsyncFinishedCallbacks = {}
      self.bindings = self._dg._createBindingList( [ name, 'bindings' ] )

    def _patch( self, diff ):
      super( _DG._NODE, self )._patch( diff )

      if 'dependencies' in diff:
        self.__dependencies = {}
        for dependencyName in diff[ 'dependencies' ]:
          dependencyNodeName = diff[ 'dependencies' ][ dependencyName ]
          self.__dependencies[ dependencyName ] = self._dg._namedObjects[ dependencyNodeName ]

      if 'bindings' in diff:
        self.bindings._patch( diff[ 'bindings' ] )

    def _route( self, src, cmd, arg ):
      if len( src ) == 1 and src[ 0 ] == 'bindings':
        src = collections.deque( src )
        src.popleft()
        self.bindings._route( src, cmd, arg )
      elif cmd == 'evaluateAsyncFinished':
        callback = self.__evaluateAsyncFinishedCallbacks[ arg ]
        del self.__evaluateAsyncFinishedCallbacks[ arg ]
        callback()
      else:
        super( _DG._NODE, self )._route( src, cmd, arg )
        
    def getType( self ):
      return 'Node'

    def __checkDependencyName( self, dependencyName ):
      try:
        if type( dependencyName ) != str:
          raise Exception( 'must be a string' )
        elif dependencyName == '':
          raise Exception( 'must not be empty' )
        elif dependencyName == 'self':
          raise Exception( 'must not be "self"' )
      except Exception as e:
        raise Exception( 'dependencyName: ' + e )

    def setDependency( self, dependencyNode, dependencyName ):
      self.__checkDependencyName( dependencyName )

      oldDependency = None
      if dependencyName in self.__dependencies:
        oldDependency = self.__dependencies[ dependencyName ]
      self.__dependencies[ dependencyName ] = dependencyNode
      
      args = { 'name': dependencyName, 'node': dependencyNode.getName() }
      def __unwind():
        if ( oldDependency is not None ):
          self.__dependencies[ dependencyName ] = oldDependency
        else:
          del self.__dependencies[ dependencyName ]
          
      self._nObjQueueCommand( 'setDependency', args, __unwind )

    def getDependencies( self ):
      return self.__dependencies

    def getDependency( self, name ):
      if name not in self.__dependencies:
        raise Exception( 'no dependency named "' + name + '"' )
      return self.__dependencies[ name ]

    def removeDependency( self, dependencyName ):
      self.__checkDependencyName( dependencyName )

      oldDependency = None
      if dependencyName in self.__dependencies:
        oldDependency = self.__dependencies[ dependencyName ]
        del self.__dependencies[ dependencyName ]
      
      def __unwind():
        if ( oldDependency is not None ):
          self.__dependencies[ dependencyName ] = oldDependency
        else:
          del self.__dependencies[ dependencyName ]
          
      self._nObjQueueCommand( 'removeDependency', dependencyName, __unwind )

    def evaluate( self ):
      self._nObjQueueCommand( 'evaluate' )
      self._dg._executeQueuedCommands()
      
    def evaluateAsync( self, callback ):
      serial = self.__evaluateAsyncFinishedSerial
      self.__evaluateAsyncFinishedSerial = self.__evaluateAsyncFinishedSerial + 1
      self.__evaluateAsyncFinishedCallbacks[ serial ] = callback
      self._nObjQueueCommand( 'evaluateAsync', serial )
      self._dg._executeQueuedCommands()
  
  class _RESOURCELOADNODE( _NODE ):
    def __init__( self, dg, name ):
      super( _DG._RESOURCELOADNODE, self ).__init__( dg, name )
      self.__onloadSuccessCallbacks = []
      self.__onloadProgressCallbacks = []
      self.__onloadFailureCallbacks = []

    def _handle( self, cmd, arg ):
      if cmd == 'resourceLoadSuccess':
        for i in range( 0, len( self.__onloadSuccessCallbacks ) ):
          self.__onloadSuccessCallbacks[ i ]( self )
      elif cmd == 'resourceLoadProgress':
        for i in range( 0, len( self.__onloadProgressCallbacks ) ):
          self.__onloadProgressCallbacks[ i ]( self, arg )
      elif cmd == 'resourceLoadFailure':
        for i in range( 0, len( self.__onloadFailureCallbacks ) ):
          self.__onloadFailureCallbacks[ i ]( self )
      else:
        super( _DG._RESOURCELOADNODE, self )._handle( cmd, arg )

    def addOnLoadSuccessCallback( self, callback ):
      self.__onloadSuccessCallbacks.append( callback )

    def addOnLoadProgressCallback( self, callback ):
      self.__onloadProgressCallbacks.append( callback )

    def addOnLoadFailureCallback( self, callback ):
      self.__onloadFailureCallbacks.append( callback )

  class _EVENT( _CONTAINER ):
    def __init__( self, dg, name ):
      super( _DG._EVENT, self ).__init__( dg, name )
      self.__eventHandlers = None
      self.__typeName = None
      self.__rt = dg._getClient().rt

    def _patch( self, diff ):
      super( _DG._EVENT, self )._patch( diff )
      self.__eventHandlers = None

      if 'eventHandlers' in diff:
        self.__eventHandlers = []
        for name in diff[ 'eventHandlers' ]:
          self.__eventHandlers.append( self._dg._namedObjects[ name ] )

    def getType( self ):
      return 'Event'

    def appendEventHandler( self, eventHandler ):
      self._nObjQueueCommand( 'appendEventHandler', eventHandler.getName() )
      self.__eventHandlers = None

    def getEventHandlers( self ):
      if self.__eventHandlers is None:
        self._dg._executeQueuedCommands()
      return self.__eventHandlers

    def fire( self ):
      self._nObjQueueCommand( 'fire' )
      self._dg._executeQueuedCommands()

    def setSelectType( self, tn ):
      self._nObjQueueCommand( 'setSelectType', tn )
      self._dg._executeQueuedCommands()
      self.__typeName = tn

    def select( self ):
      data = []
      def __callback( results ):
        for i in range( 0, len( results ) ):
          result = results[ i ]
          data.append( {
            'node': self._dg._namedObjects[ result[ 'node' ] ],
            'value': self.__rt._assignPrototypes( result[ 'data' ], self.__typeName )
          })

      self._nObjQueueCommand( 'select', self.__typeName, None, __callback )
      self._dg._executeQueuedCommands()
      return data

  class _EVENTHANDLER( _CONTAINER ):
    def __init__( self, dg, name ):
      super( _DG._EVENTHANDLER, self ).__init__( dg, name )
      self.__scopes = {}
      self.__bindingName = None
      self.__childEventHandlers = None
      self.preDescendBindings = self._dg._createBindingList( [ name, 'preDescendBindings' ] )
      self.postDescendBindings = self._dg._createBindingList( [ name, 'postDescendBindings' ] )

    def _patch( self, diff ):
      super( _DG._EVENTHANDLER, self )._patch( diff )
      if 'bindingName' in diff:
        self.__bindingName = diff[ 'bindingName' ]

      if 'childEventHandlers' in diff:
        self.__childEventHandlers = []
        for name in diff[ 'childEventHandlers' ]:
          self.__childEventHandlers.append( self._dg._namedObjects[ name ] )

      if 'scopes' in diff:
        self.__scopes = {}
        for name in diff[ 'scopes' ]:
          nodeName = diff[ 'scopes' ][ name ]
          self.__scopes[ name ] = self._dg._namedObjects[ nodeName ]

      if 'preDescendBindings' in diff:
        self.preDescendBindings._patch( diff[ 'preDescendBindings' ] )

      if 'postDescendBindings' in diff:
        self.postDescendBindings._patch( diff[ 'postDescendBindings' ] )

    def _route( self, src, cmd, arg ):
      if len( src ) == 1 and src[ 0 ] == 'preDescendBindings':
        src = collections.deque( src )
        src.popleft()
        self.preDescendBindings._route( src, cmd, arg )
      elif len( src ) == 1 and src[ 0 ] == 'postDescendBindings':
        src = collections.deque( src )
        src.popleft()
        self.postDescendBindings._route( src, cmd, arg )
      else:
        super( _DG._EVENTHANDLER, self )._route( src, cmd, arg )
    
    def getType( self ):
      return 'EventHandler'

    def getScopeName( self ):
      return self.__bindingName

    def setScopeName( self, bindingName ):
      oldBindingName = self.__bindingName
      def __unwind():
        self.__bindingName = oldBindingName
      self._nObjQueueCommand( 'setScopeName', bindingName, __unwind )

    def appendChildEventHandler( self, childEventHandler ):
      oldChildEventHandlers = self.__childEventHandlers
      self.__childEventHandlers = None
      def __unwind():
        self.__childEventHandlers = oldChildEventHandlers
      self._nObjQueueCommand( 'appendChildEventHandler', childEventHandler.getName(), __unwind )

    def removeChildEventHandler( self, childEventHandler ):
      oldChildEventHandlers = self.__childEventHandlers
      self.__childEventHandlers = None
      def __unwind():
        self.__childEventHandlers = oldChildEventHandlers
      self._nObjQueueCommand( 'removeChildEventHandler', childEventHandler.getName(), __unwind )

    def getChildEventHandlers( self ):
      if self.__childEventHandlers is None:
        self._dg._executeQueuedCommands()
      return self.__childEventHandlers

    def __checkScopeName( self, name ):
      try:
        if type( name ) != str:
          raise Exception( 'must be a string' )
        elif name == '':
          raise Exception( 'must not be empty' )
      except Exception as e:
        raise Exception( 'name: ' + e )

    def setScope( self, name, node ):
      self.__checkScopeName( name )

      oldNode = None
      if name in self.__scopes:
        oldNode = self.__scopes[ name ]
      self.__scopes[ name ] = node

      def __unwind():
        if oldNode is not None:
          self.__scopes[ name ] = oldNode
        else:
          del self.__scopes[ name ]
      args = { 'name': name, 'node': node.getName() }
      self._nObjQueueCommand( 'setScope', args, __unwind )

    def removeScope( self, name ):
      self.__checkScopeName( name )

      oldNode = None
      if name in self.__scopes:
        oldNode = self.__scopes[ name ]
        del self.__scopes[ name ]
    
      def __unwind():
        if oldNode is not None:
          self.__scopes[ name ] = oldNode
      self._nObjQueueCommand( 'removeScope', name, __unwind )

    def getScopes( self ):
      return self.__scopes

    def setSelector( self, targetName, binding ):
      operatorName = None
      try:
        operatorName = binding.getOperator().getName()
      except Exception:
        raise Exception( 'operator: not an operator' )

      args = {
        'targetName': targetName,
        'operator': operatorName,
        'parameterLayout': binding.getParameterLayout()
      }
      self._nObjQueueCommand( 'setSelector', args )

class _MR( _NAMESPACE ):
  def __init__( self, client ):
    super( _MR, self ).__init__( client, 'MR' )

  def createConstArray( self, elementType, data = None ):
    valueArray = self._ARRAYPRODUCER( self )

    arg = { 'id': valueArray.getID() }
    if type( elementType ) is str:
      arg[ 'elementType' ] = elementType
      arg[ 'data' ] = data
    elif type ( elementType ) is dict:
      inputArg = elementType
      arg[ 'elementType' ] = inputArg[ 'elementType' ]
      if 'data' in inputArg:
        arg[ 'data' ] = inputArg[ 'data' ]
      if 'jsonData' in inputArg:
        arg[ 'jsonData' ] = inputArg[ 'jsonData' ]
    else:
      raise Exception( "createConstArray: first argument must be str or dict" )

    self._queueCommand( 'createConstArray', arg, valueArray.unwind )
    return valueArray

  def createConstValue( self, valueType, data ):
    value = self._VALUEPRODUCER( self )
    arg = {
      'id': value.getID(),
      'valueType': valueType,
      'data': data
    }
    self._queueCommand( 'createConstValue', arg, value.unwind )
    return value

  def createValueCache( self, input ):
    return self.__createMRCommand( self._VALUEPRODUCER( self ), 'createValueCache', input, None, None )

  def createValueGenerator( self, operator ):
    return self.__createMRCommand( self._VALUEPRODUCER( self ), 'createValueGenerator', None, operator, None )

  def createValueMap( self, input, operator, shared = None ):
    return self.__createMRCommand( self._VALUEPRODUCER( self ), 'createValueMap', input, operator, shared )

  def createValueTransform( self, input, operator, shared = None ):
    return self.__createMRCommand( self._VALUEPRODUCER( self ), 'createValueTransform', input, operator, shared )

  def createArrayCache( self, input ):
    return self.__createMRCommand( self._ARRAYPRODUCER( self ), 'createArrayCache', input, None, None )

  def createArrayGenerator( self, count, operator, shared = None ):
    obj = self._ARRAYPRODUCER( self )
    arg = {
      'id': obj.getID(),
      'countID': count.getID(),
      'operatorID': operator.getID()
    }
    if ( shared is not None ):
      arg[ 'sharedID' ] = shared.getID()

    self._queueCommand( 'createArrayGenerator', arg, obj.unwind )
    return obj

  def createArrayMap( self, input, operator, shared = None ):
    return self.__createMRCommand( self._ARRAYPRODUCER( self ), 'createArrayMap', input, operator, shared )

  def createArrayTransform( self, input, operator, shared = None ):
    return self.__createMRCommand( self._ARRAYPRODUCER( self ), 'createArrayTransform', input, operator, shared )

  def createReduce( self, inputArrayProducer, reduceOperator, sharedValueProducer = None ):
    reduce = self._VALUEPRODUCER( self )
    arg = {
      'id': reduce.getID(),
      'inputID': inputArrayProducer.getID(),
      'operatorID': reduceOperator.getID()
    }
    if ( sharedValueProducer is not None ):
      arg[ 'sharedID' ] = sharedValueProducer.getID()

    self._queueCommand( 'createReduce', arg, reduce.unwind )
    return reduce

  def __createMRCommand( self, obj, cmd, input, operator, shared ):
    arg = {
      'id': obj.getID()
    }
    if ( input is not None ):
      arg[ 'inputID' ] = input.getID()
    if ( operator is not None ):
      arg[ 'operatorID' ] = operator.getID()
    if ( shared is not None ):
      arg[ 'sharedID' ] = shared.getID()

    self._queueCommand( cmd, arg, obj.unwind )
    return obj

  class _PRODUCER( _GCOBJECT ):
    def __init__( self, mr ):
      super( _MR._PRODUCER, self ).__init__( mr )
  
    def toJSON( self ):
      # dictionary hack to simulate Python 3.x nonlocal
      json = { '_': None }
      def __toJSON( result ):
        json[ '_' ] = result
  
      self._gcObjQueueCommand( 'toJSON', None, None, __toJSON )
      return json[ '_' ]
  
  class _ARRAYPRODUCER( _PRODUCER ):
    def __init__( self, mr ):
      super( _MR._ARRAYPRODUCER, self ).__init__( mr )
  
    def getCount( self ):
      # dictionary hack to simulate Python 3.x nonlocal
      count = { '_': None }
      def __getCount( result ):
        count[ '_' ] = result
  
      self._gcObjQueueCommand( 'getCount', None, None, __getCount )
      self._nsobj._executeQueuedCommands()
      return count[ '_' ]
  
    def produce( self, index = None, count = None ):
      arg = { }
      if ( index is not None ):
        if ( count is not None ):
          arg[ 'count' ] = count
        arg[ 'index' ] = index
  
      # dictionary hack to simulate Python 3.x nonlocal
      result = { '_': None }
      def __produce( data ):
        result[ '_' ] = data
  
      self._gcObjQueueCommand( 'produce', arg, None, __produce )
      self._nsobj._executeQueuedCommands()
      return result[ '_' ]
  
    def flush( self ):
      self._gcObjQueueCommand( 'flush' )
  
    def produceAsync( self, arg1, arg2 = None, arg3 = None ):
      arg = { }
      callback = None
      if arg3 is None and arg2 is None:
        callback = arg1
      elif arg3 is None:
        arg[ 'index' ] = arg1
        callback = arg2
      else:
        arg[ 'index' ] = arg1
        arg[ 'count' ] = arg2
        callback = arg3
  
      arg[ 'serial' ] = self._registerCallback( callback )
      self._gcObjQueueCommand( 'produceAsync', arg )
      self._nsobj._executeQueuedCommands()
  
  class _VALUEPRODUCER( _PRODUCER ):
    def __init__( self, client ):
      super( _MR._VALUEPRODUCER, self ).__init__( client )
  
    def produce( self ):
      # dictionary hack to simulate Python 3.x nonlocal
      result = { '_': None }
      def __produce( data ):
        result[ '_' ] = data
  
      self._gcObjQueueCommand( 'produce', None, None, __produce )
      self._nsobj._executeQueuedCommands()
      return result[ '_' ]
  
    def produceAsync( self, callback ):
      self._gcObjQueueCommand( 'produceAsync', self._registerCallback( callback ) )
      self._nsobj._executeQueuedCommands()
  
    def flush( self ):
      self._gcObjQueueCommand( 'flush' )
  
class _KLC( _NAMESPACE ):
  def __init__( self, client ):
    super( _KLC, self ).__init__( client, 'KLC' )

  def createCompilation( self, sourceName = None, sourceCode = None ):
    obj = self._COMPILATION( self )
    arg = { 'id': obj.getID() }
    if sourceName is not None:
      arg[ 'sourceName' ] = sourceName
    if sourceCode is not None:
      arg[ 'sourceCode' ] = sourceCode
    self._queueCommand( 'createCompilation', arg, obj.unwind )
    return obj

  def _createExecutable( self ):
    return self._EXECUTABLE( self )

  def createExecutable( self, sourceName, sourceCode ):
    obj = self._createExecutable()
    arg = {
      'id': obj.getID(),
      'sourceName': sourceName,
      'sourceCode': sourceCode
    }
    self._queueCommand( 'createExecutable', arg, obj.unwind )
    return obj

  def _createOperatorOnly( self ):
    operator = self._OPERATOR( self )
    return operator

  def _createOperator( self, operatorName, cmd, sourceName = None, sourceCode = None ):
    operator = self._createOperatorOnly()
    arg = {
      'id': operator.getID(),
      'operatorName': operatorName,
      'sourceName': sourceName,
      'sourceCode': sourceCode
    }
    self._queueCommand( cmd, arg, operator.unwind )
    return operator

  def createReduceOperator( self, sourceName, sourceCode, operatorName ):
    return self._createOperator( operatorName, 'createReduceOperator', sourceName, sourceCode )

  def createValueGeneratorOperator( self, sourceName, sourceCode, operatorName ):
    return self._createOperator( operatorName, 'createValueGeneratorOperator', sourceName, sourceCode )

  def createValueMapOperator( self, sourceName, sourceCode, operatorName ):
    return self._createOperator( operatorName, 'createValueMapOperator', sourceName, sourceCode )

  def createValueTransformOperator( self, sourceName, sourceCode, operatorName ):
    return self._createOperator( operatorName, 'createValueTransformOperator', sourceName, sourceCode )

  def createArrayGeneratorOperator( self, sourceName, sourceCode, operatorName ):
    return self._createOperator( operatorName, 'createArrayGeneratorOperator', sourceName, sourceCode )

  def createArrayMapOperator( self, sourceName, sourceCode, operatorName ):
    return self._createOperator( operatorName, 'createArrayMapOperator', sourceName, sourceCode )

  def createArrayTransformOperator( self, sourceName, sourceCode, operatorName ):
    return self._createOperator( operatorName, 'createArrayTransformOperator', sourceName, sourceCode )

  class _OPERATOR( _GCOBJECT ):
    def __init__( self, klc ):
      super( _KLC._OPERATOR, self ).__init__( klc )

    def toJSON( self ):
      return self._synchronousGetOnly( 'toJSON' )
 
    def getDiagnostics( self ):
      return self._synchronousGetOnly( 'getDiagnostics' )

  class _EXECUTABLE( _GCOBJECT ):
    def __init__( self, klc ):
      super( _KLC._EXECUTABLE, self ).__init__( klc )

    def __resolveOperator( self, operatorName, cmd ):
      operator = self._nsobj._createOperatorOnly()
      arg = {
        'id': operator.getID(),
        'operatorName': operatorName
      }
      self._gcObjQueueCommand( cmd, arg, operator.unwind )
      return operator

    def getAST( self ):
      return self._synchronousGetOnly( 'getAST' )

    def getDiagnostics( self ):
      return self._synchronousGetOnly( 'getDiagnostics' )
 
    def resolveReduceOperator( self, operatorName ):
      return self.__resolveOperator( operatorName, 'resolveReduceOperator' )
  
    def resolveValueGeneratorOperator( self, operatorName ):
      return self.__resolveOperator( operatorName, 'resolveValueGeneratorOperator' )
  
    def resolveValueMapOperator( self, operatorName ):
      return self.__resolveOperator( operatorName, 'resolveValueMapOperator' )
  
    def resolveValueTransformOperator( self, operatorName ):
      return self.__resolveOperator( operatorName, 'resolveValueTransformOperator' )
  
    def resolveArrayGeneratorOperator( self, operatorName ):
      return self.__resolveOperator( operatorName, 'resolveArrayGeneratorOperator' )
  
    def resolveArrayMapOperator( self, operatorName ):
      return self.__resolveOperator( operatorName, 'resolveArrayMapOperator' )

    def resolveArrayTransformOperator( self, operatorName ):
      return self.__resolveOperator( operatorName, 'resolveArrayTransformOperator' )

  class _COMPILATION( _GCOBJECT ):
    def __init__( self, klc ):
      super( _KLC._COMPILATION, self ).__init__( klc )
      self.__sourceCodes = {}

    def addSource( self, sourceName, sourceCode ):
      oldSourceCode = None
      if sourceName in self.__sourceCodes:
        oldSourceCode = self.__sourceCodes[ sourceName ]

      self.__sourceCodes[ sourceName ] = sourceCode

      def __unwind():
        if oldSourceCode is not None:
          self.__sourceCodes[ sourceName ] = oldSourceCode
        else:
          del self.__sourceCodes[ sourceName ]

      args = { 'sourceName': sourceName, 'sourceCode': sourceCode }
      self._gcObjQueueCommand( 'addSource', args, __unwind )

    def removeSource( self, sourceName ):
      oldSourceCode = None
      if sourceName in self.__sourceCodes:
        oldSourceCode = self.__sourceCodes[ sourceName ]
        del self.__sourceCodes[ sourceName ]

      def __unwind():
        if oldSourceCode is not None:
          self.__sourceCodes[ sourceName ] = oldSourceCode

      args = { 'sourceName': sourceName }
      self._gcObjQueueCommand( 'removeSource', args, __unwind )
     
    def getSources( self ):
      # dictionary hack to simulate Python 3.x nonlocal
      data = { '_': None }
      def __callback( result ):
        data[ '_' ] = result
  
      self._gcObjQueueCommand( 'getSources', None, None, __callback )
      self._nsobj._executeQueuedCommands()
      return data[ '_' ]

    def run( self ):
      executable = self._nsobj._createExecutable()
      args = { 'id': executable.getID() }
      self._gcObjQueueCommand( 'run', args, executable.unwind )
      return executable

class _RT( _NAMESPACE ):
  def __init__( self, client ):
    super( _RT, self ).__init__( client, 'RT' )
    self.__prototypes = {}
    self.__registeredTypes = {}

  def _assignPrototypes( self, data, typeName ):
    if typeName[-2:] == '[]':
      obj = []
      typeName = typeName[0:-2]
      for i in range( 0, len( data ) ):
        obj.append( self._assignPrototypes( data[ i ], typeName ) )
      return obj

    elif typeName in self.__prototypes:
      obj = self.__prototypes[ typeName ]()
      if 'members' in self.__registeredTypes[ typeName ]:
        members = self.__registeredTypes[ typeName ][ 'members' ]
        for i in range( 0, len( members ) ):
          member = members[ i ]
          setattr( obj, member[ 'name' ],
            self._assignPrototypes( data[ member[ 'name' ] ], member[ 'type' ] )
          )
      return obj

    else:
      return data
    
  def getRegisteredTypes( self ):
    self._executeQueuedCommands()
    return self.__registeredTypes

  def registerType( self, name, desc ):
    if type( desc ) is not dict:
      raise Exception( 'RT.registerType: second parameter: must be an object' )
    if 'members' not in desc:
      raise Exception( 'RT.registerType: second parameter: missing members element' )
    if type( desc[ 'members' ] ) is not list:
      raise Exception( 'RT.registerType: second parameter: invalid members element' )

    members = []
    for i in range( 0, len( desc[ 'members' ] ) ):
      member = desc[ 'members' ][ i ]
      memberName, memberType =  member.popitem()
      if len( member ) > 0:
        raise Exception( 'improperly formatted member' )
      member = {
        'name': memberName,
        'type': memberType
      }
      members.append( member )

    constructor = None
    if 'constructor' in desc:
      constructor = desc[ 'constructor' ]
    else:
      class _Empty:
        pass
      constructor = _Empty

    defaultValue = constructor()
    self.__prototypes[ name ] = constructor

    arg = {
      'name': name,
      'members': members,
      'defaultValue': _typeToDict( defaultValue )
    }
    if ( 'klBindings' in desc ):
      arg[ 'klBindings' ] = desc[ 'klBindings' ]

    def __unwind():
      del self.__prototypes[ name ]
    self._queueCommand( 'registerType', arg, __unwind )

  def _patch( self, diff ):
    if 'registeredTypes' in diff:
      self.__registeredTypes = {}
      for typeName in diff[ 'registeredTypes' ]:
        self.__registeredTypes[ typeName ] = diff[ 'registeredTypes' ][ typeName ]

  def _handleStateNotification( self, state ):
    self.__prototypes = {}
    self._patch( state )

  def _handle( self, cmd, arg ):
    if cmd == 'delta':
      self._patch( arg )
    else:
      raise Exception( 'command "' + cmd + '": unrecognized' )
  
  def _route( self, src, cmd, arg ):
    if len( src ) == 0:
      self._handle( cmd, arg )
    elif len( src ) == 1:
      typeName = src[ 0 ]
      try:
        if cmd == 'delta':
          self.__registeredTypes[ typeName ] = arg
          self.__registeredTypes[ typeName ][ 'defaultValue' ] = self._assignPrototypes(
              self.__registeredTypes[ typeName ][ 'defaultValue' ],
              typeName
            )
        else:
          raise Exception( 'unrecognized' )
      except Exception as e:
        raise Exception( '"' + cmd + '": ' + e )
    else:
      raise Exception( '"' + src + '": unroutable ' )

class _GC( _NAMESPACE ):
  def __init__( self, client ):
    super( _GC, self ).__init__( client, 'GC' )
    self.__objects = {}

  def addObject( self, obj ):
    self.__objects[ obj.getID() ] = obj

  def disposeObject( self, obj ):
    del self.__objects[ obj.getID() ]

  def _route( self, src, cmd, arg ):
    src = collections.deque( src )
    id = src.popleft()
    obj = self.__objects[ id ]
    obj._route( src, cmd, arg )

class _EX( _NAMESPACE ):
  def __init__( self, client ):
    super( _EX, self ).__init__( client, 'EX' )
    self.__loadedExts = {}

  def _patch( self, diff ):
    for name in diff:
      if diff[ name ]:
        self.__loadedExts = diff[ name ]
      elif name in self.__loadedExts:
        del self.__loadedExts[ name ]

  def _handleStateNotification( self, state ):
    self.__loadedExts = {}
    self._patch( state )

  def _handle( self, cmd, arg ):
    if cmd == 'delta':
      self._patch( arg )
    else:
      raise Exception( 'command "' + cmd + '": unrecognized' )

  def _route( self, src, cmd, arg ):
    if len( src ) > 0:
      self._handle( cmd, arg )
    else:
      raise Exception( 'unroutable' )

  def getLoadedExts( self ):
    return self.__loadedExts

class _IO( _NAMESPACE ):
  def __init__( self, client ):
    super( _IO, self ).__init__( client, 'IO' )

    self.forOpen = 'openMode'
    self.forOpenWithWriteAccess = 'openWithWriteAccessMode'
    self.forSave = 'saveMode'

  def __queryUserFile( self, funcname, mode, uiTitle, extension, defaultFileName ):
    if mode != self.forOpen and mode != self.forOpenWithWriteAccess and mode != self.forSave:
      raise Exception( 'Invalid mode: "' + mode + '": can be IO.forOpen, IO.forOpenWithWriteAccess or IO.forSave' )

    # dictionary hack to simulate Python 3.x nonlocal
    data = { '_': None }
    def __callback( result ):
      data[ '_' ] = result
    args = {
      'existingFile': mode == self.forOpenWithWriteAccess or mode == self.forOpen,
      'writeAccess': mode == self.forOpenWithWriteAccess or mode == self.forSave,
      'uiOptions': {
        'title': uiTitle,
        'extension': extension,
        'defaultFileName': defaultFileName
      }
    }
    self._queueCommand( funcname, args, None, __callback )
    self._executeQueuedCommands()
    return data[ '_' ]

  def queryUserFileAndFolderHandle( self, mode, uiTitle, extension, defaultFileName ):
    return self.__queryUserFile( 'queryUserFileAndFolder', mode, uiTitle, extension, defaultFileName )

  def queryUserFileHandle( self, mode, uiTitle, extension, defaultFileName ):
    return self.__queryUserFile( 'queryUserFile', mode, uiTitle, extension, defaultFileName )

  def getTextFileContent( self, handle ):
    # dictionary hack to simulate Python 3.x nonlocal
    data = { '_': None }
    def __callback( result ):
      data[ '_' ] = result
    self._queueCommand( 'getTextFileContent', handle, None, __callback )
    self._executeQueuedCommands()
    return data[ '_' ]

  def putTextFileContent( self, handle, content, append = None ):
    args = {
      'content': content,
      'file': handle,
      'append': False if append is None else append
    }
    self._queueCommand( 'putTextFileContent', args )
    self._executeQueuedCommands()

  def buildFileHandleFromRelativePath( self, handle ):
    # dictionary hack to simulate Python 3.x nonlocal
    data = { '_': None }
    def __callback( result ):
      data[ '_' ] = result
    self._queueCommand( 'createFileHandleFromRelativePath', handle, None, __callback )
    self._executeQueuedCommands()
    return data[ '_' ]
    
  def buildFolderHandleFromRelativePath( self, handle ):
    # dictionary hack to simulate Python 3.x nonlocal
    data = { '_': None }
    def __callback( result ):
      data[ '_' ] = result
    self._queueCommand( 'createFolderHandleFromRelativePath', handle, None, __callback )
    self._executeQueuedCommands()
    return data[ '_' ]
    
  def getFileHandleInfo( self, handle ):
    # dictionary hack to simulate Python 3.x nonlocal
    data = { '_': None }
    def __callback( result ):
      data[ '_' ] = result
    self._queueCommand( 'getFileInfo', handle, None, __callback )
    self._executeQueuedCommands()
    return data[ '_' ]

class _BUILD( _NAMESPACE ):
  def __init__( self, client ):
    super( _BUILD, self ).__init__( client, 'build' )
    self.__build = {}

  def _handleStateNotification( self, state ):
    pass

  def _patch( self, diff ):
    for name in diff:
      self.__build[ name ] = diff[ name ]

  def _handleStateNotification( self, state ):
    self._patch( state )

  def _handle( self, cmd, arg ):
    if cmd == 'delta':
      self._patch( arg )
    else:
      raise Exception( 'command "' + cmd + '": unrecognized' )

  def _route( self, src, cmd, arg ):
    if len( src ) == 0:
      self._handle( cmd, arg )
    else:
      raise Exception( 'unroutable' )

  def isExpired( self ):
    return self.__build[ 'isExpired' ]

  def getName( self ):
    return self.__build[ 'name' ]

  def getPureVersion( self ):
    return self.__build[ 'pureVersion' ]

  def getFullVersion( self ):
    return self.__build[ 'fullVersion' ]

  def getDesc( self ):
    return self.__build[ 'desc' ]

  def getCopyright( self ):
    return self.__build[ 'copyright' ]

  def getURL( self ):
    return self.__build[ 'url' ]

  def getOS( self ):
    return self.__build[ 'os' ]

  def getArch( self ):
    return self.__build[ 'arch' ]

class _VP( _NAMESPACE ):
  def __init__( self, client ):
    super( _VP, self ).__init__( client, 'VP' )
    self.__viewPorts = {}

  def _handleStateNotification( self, state ):
    pass

  def _route( self, src, cmd, arg ):
    pass

