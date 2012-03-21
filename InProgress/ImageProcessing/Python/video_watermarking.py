import fabric
import math
import sys
import time

args = sys.argv
if not len(args) == 4:
  print "---------"
  print "Please specify the command line arguments like this:"
  print "  python video_watermarking.py input_video_file watermark_image_file output_video_file"
  print "---------"
  exit()

inputMovie = args[1]
watermarkImage = args[2]
outputMovie = args[3]

# create a context and store it
F = fabric.createClient()

# define a helper function to construct an operator
def createOperator( node, options ):
  # check input arguments
  if 'name' not in options:
    raise Exception( 'You need to specify an operator name!' )
  if 'srcCode' not in options:
    raise Exception( 'You need to specify an operator sourceCode!' )
  if 'binding' not in options:
    raise Exception( 'You need to specify an operator binding!' )

  # create the operator
  operator = F.DG.createOperator( options[ 'name' ] )
  operator.setSourceCode( options[ 'srcCode' ] )
  operator.setEntryFunctionName( options[ 'name' ] )
  operator.setMainThreadOnly( True )
  diags = operator.getDiagnostics()
  if len(diags) > 0:
    for diag in diags:
      print diag['filename']+":"+str(diag['line'])+":"+str(diag['column'])+": "+diag['level']+": "+diag['desc']
    raise Exception("Errors compiling '" + options['name'] + "':")
  
  # create a binding between the node and the operator
  binding = F.DG.createBinding()
  binding.setOperator( operator )
  binding.setParameterLayout( options[ 'binding' ] )

  # append the new binding to the node
  node.bindings.append( binding );
  errors = node.getErrors();
  if len( errors ) > 0:
    raise Exception( errors )

# create the dependency graph nodes
watermarkNode = F.DG.createNode( 'watermark' )
videoInputNode = F.DG.createNode( 'videoInput' )
videoOutputNode = F.DG.createNode( 'videoOutput' )

# create dependencies between them
videoOutputNode.setDependency( videoInputNode, 'input' )
videoOutputNode.setDependency( watermarkNode, 'watermark' );

# create the watermark image members
watermarkNode.addMember('filePath','String',watermarkImage)
watermarkNode.addMember('ext','String',watermarkImage.rpartition('.')[2])
watermarkNode.addMember('clientData','String','Client Name & Address')
watermarkNode.addMember('width','Size',0)
watermarkNode.addMember('height','Size',0)
watermarkNode.addMember('pixels','RGBA[]')

# create the input video members
videoInputNode.addMember('filePath','String',inputMovie)
videoInputNode.addMember('video','VideoHandle')
videoInputNode.addMember('pixels','RGB[]')
videoInputNode.addMember('time','Scalar',0.0)

# create the output video members
videoOutputNode.addMember('filePath','String',outputMovie)
videoOutputNode.addMember('video','VideoHandle')
videoOutputNode.addMember('offsetWidth','Scalar',0.0)
videoOutputNode.addMember('offsetHeight','Scalar',0.0)
videoOutputNode.addMember('directionWidth','Scalar',1.0)
videoOutputNode.addMember('directionHeight','Scalar',1.0)
videoOutputNode.addMember('pixels','RGB[]')

# create the watermark operator
createOperator(watermarkNode, {
  'name': 'openWaterMark',
  'srcCode': open( 'openWaterMark.kl' ).read(),
  'binding': [
    'self.filePath',
    'self.ext',
    'self.clientData',
    'self.width',
    'self.height',
    'self.pixels'
  ]
})

# create the open video operator
createOperator(videoInputNode,{
  'name': 'openVideoFile',
  'srcCode': open( 'openVideoFile.kl' ).read(),
  'binding': [
    'self.filePath',
    'self.video'
  ]
})

# create the seek video operator
createOperator(videoInputNode,{
  'name': 'seekVideoFile',
  'srcCode': open( 'seekVideoFile.kl' ).read(),
  'binding': [
    'self.video',
    'self.pixels',
    'self.time'
  ]
})

# create the output video create operator
createOperator(videoOutputNode,{
  'name': 'createVideoOutput',
  'srcCode': open( 'createVideoOutput.kl' ).read(),
  'binding': [
    'self.video',
    'self.filePath',
    'input.video'
  ]
})

# create the compositing operator  
createOperator(videoOutputNode,{
  'name': 'compositeWatermark',
  'srcCode': open( 'compositeWatermark.kl' ).read(),
  'binding': [
    'self.video',
    'input.video',
    'input.pixels',
    'self.pixels',
    'watermark.pixels',
    'watermark.width',
    'watermark.height',
    'self.offsetWidth',
    'self.offsetHeight',
    'self.directionWidth',
    'self.directionHeight'
  ]
})

# create the output video add frame operator
createOperator(videoOutputNode,{
  'name': 'writeVideoFrame',
  'srcCode': open( 'writeVideoFrame.kl' ).read(),
  'binding': [
    'self.video',
    'self.pixels',
    'input.time'
  ]
})

# evaluate the nodes!
videoOutputNode.evaluate()

videoData = videoInputNode.getData('video',0)
fps = videoData[ 'fps' ]
duration = videoData[ 'duration' ]
ttime = 0.0

# output some standard information
print('--------------------------------------')
print('Input video: '+videoInputNode.getData('filePath',0))
print('WaterMark: '+watermarkNode.getData('filePath',0))
print('Output video: '+videoOutputNode.getData('filePath',0))
print('Duration: '+str(duration)+' seconds.')
print('--------------------------------------')

# output a progress bar
lastLogTime = -1;
def logProgress():
  global lastLogTime
  logTime = math.floor(100.0 * ttime / duration)
  if logTime != lastLogTime:
    prog = ''
    while len( prog ) < logTime * 0.5:
      prog += '#'
    while len( prog ) < 50:
      prog += ' '
    sys.stdout.write('\r --> Converting: ['+prog+'] '+str(logTime)+'%.')
    sys.stdout.flush()
    lastLogTime = logTime
logProgress()

# increment the time, to render each frame one after another
def incrementTime():
  global ttime
  if ttime >= duration:
    logProgress()
    F.close()
    print('\ndone.')
    return

  ttime += 1.0 / fps
  videoInputNode.setData('time',0,ttime)
  videoOutputNode.evaluate()
  logProgress()

while F.running():
  incrementTime()
  time.sleep( 0.01 )

