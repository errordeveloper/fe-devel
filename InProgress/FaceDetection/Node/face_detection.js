// create a context and store it
F = require('Fabric').createClient();

// define a helper function to construct an operator
var createOperator = function(node,options) {
  
  // check input arguments
  if(options.name == undefined)
    throw('You need to specify an operator name!');
  if(options.srcCode == undefined)
    throw('You need to specify an operator sourcecode!');
  if(options.binding == undefined)
    throw('You need to specify an operator binding!');

  // create the operator
  var operator = F.DG.createOperator(options.name);
  operator.setSourceCode(options.srcCode.join('\n'));
  operator.setEntryFunctionName(options.name);
  operator.setMainThreadOnly(true);
  
  // create a binding between the node and the operator
  var binding = F.DG.createBinding();
  binding.setOperator(operator);
  binding.setParameterLayout(options.binding);

  // append the new binding to the node
  node.bindings.append(binding);
  var errors = node.getErrors();
  if(errors.length) {
    throw(errors);
  }
};

if(process.argv.length != 4) {
  console.log("---------");
  console.log("Please specify the command line arguments like this:");
  console.log("  "+process.argv[0]+" "+process.argv[1]+" input_video_file output_detector_folder");
  console.log("---------");
  process.exit();
}  

// create the input video node 
var videoInputNode = F.DG.createNode("videoInput");
videoInputNode.addMember('videoPath','String',process.argv[2]);
videoInputNode.addMember('video','VideoHandle');
videoInputNode.addMember('pixels','RGB[]');
videoInputNode.addMember('time','Scalar',0.0);
videoInputNode.addMember('detector','FaceDetector');
videoInputNode.addMember('detectorPath','String','haarcascade_frontalface_alt.xml');
videoInputNode.addMember('detectorFolder','String',process.argv[3]);
videoInputNode.addMember('jpgPaths','String[]');

// create the face recognition node
var faceRecognitionNode = F.DG.createNode("faceRecognition");

// create the open video operator
createOperator(videoInputNode,{
  name: 'videoStream',
  srcCode: [
    'use FabricFILESYSTEM;',
    'use FabricVIDEO;',
    'use FabricFACE;',
    'use FabricCIMG;',
    '',
    'operator videoStream(',
    '  io String videoPath,',
    '  io VideoHandle video,',
    '  io RGB pixels[],',
    '  io Scalar time,',
    '  io FaceDetector detector,',
    '  io String detectorPath,',
    '  io String detectorFolder,',
    '  io String jpgPaths[]',
    ') {',
    '  if(!video.pointer) {',
    '    FabricFileHandleWrapper wrapper;',
    '    wrapper.setAbsolutePath(videoPath);',
    '    FabricVIDEOOpenFileHandle(wrapper.getHandle(), video);',
    '    report("Loaded video handle, resolution "+video.width+"x"+video.height+".");',
    '  }',
    '  if(video.pointer) {',
    '    FabricVIDEOSeekTime(video,time);',
    '  }',
    '  if(!detector.pointer) {',
    '    FabricFileHandleWrapper wrapper;',
    '    wrapper.setAbsolutePath(detectorPath);',
    '    detector.init(wrapper.getHandle());',
    '  }',
    '  if(detector.pointer) {',
    '    FabricVIDEOGetAllPixels(video,pixels);',
    '    FaceLocation faces[];',
    '    detector.detectRGB(pixels,video.width,video.height,1.5,Size(40),Size(40),faces);',
    '    for(Size i=0;i<faces.size();i++) {',
    '      String jpgPath = detectorFolder+"/face_"+jpgPaths.size()+".jpg";',
    '      jpgPaths.push(jpgPath);',
    '      Size width = faces[i].right - faces[i].left;',
    '      Size height = faces[i].top - faces[i].bottom;',
    '      Size offset = 0;',
    '      RGBA faceImg[];',
    '      faceImg.resize(width * height);',
    '      for(Size y=0;y<height;y++) {',
    '        for(Size x=0;x<width;x++) {',
    '          RGB pixel = pixels[(y+faces[i].bottom) * video.width + x + faces[i].left];',
    '          faceImg[offset].r = pixel.r;',
    '          faceImg[offset].g = pixel.g;',
    '          faceImg[offset].b = pixel.b;',
    '          faceImg[offset].a = 255;',
    '          offset++;',
    '        }',
    '      }',
    '      FabricFileHandleWrapper wrapper;',
    '      wrapper.setAbsolutePath(jpgPath);',
    '      FabricCIMGSaveToFileHandle(wrapper.getHandle(),width,height,false,faceImg);',
    '    }',
    '  }',
    '}'
  ],
  binding: [
    'self.videoPath',
    'self.video',
    'self.pixels',
    'self.time',
    'self.detector',
    'self.detectorPath',
    'self.detectorFolder',
    'self.jpgPaths'
  ]
});

// evaluate the nodes!
videoInputNode.evaluate();

var videoData = videoInputNode.getData('video',0);
var fps = videoData.fps;
var duration = videoData.duration;
var time = 0.0;

// output some standard information
console.log('--------------------------------------');
console.log('Input video: '+videoInputNode.getData('videoPath',0));
console.log('Duration: '+duration+' seconds.');
console.log('--------------------------------------');

// output a progress bar
var lastLogTime = -1;
var logProgress = function() {
  var logTime = Math.floor(100.0 * time / duration);
  if(logTime != lastLogTime) {
    var prog = '';
    while(prog.length < logTime * 0.5)
      prog += '#'
    while(prog.length < 50)
      prog += ' '
    process.stdout.write('\r --> Scanning: ['+prog+'] '+(logTime)+'%, '+Math.floor(time*fps)+' frames.');
    lastLogTime = logTime;
  }
}
logProgress();

// increment the time, to render each frame one after another
var incrementTime = function() {
  if(time >= duration) {
    logProgress();
    console.log('\ndone.')
    F.close();
    return;
  }
  
  setTimeout(function(){
    time += 15.0 / fps;
    videoInputNode.setData('time',0,time);
    videoInputNode.evaluate();
    logProgress();
    incrementTime();
  }, 1);
};
incrementTime();

