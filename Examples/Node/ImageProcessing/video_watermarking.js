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

// create the dependency graph nodes
var watermarkNode = F.DG.createNode("watermark");
var videoInputNode = F.DG.createNode("videoInput");
var videoOutputNode = F.DG.createNode("videoOutput");

// create dependencies between them
videoOutputNode.setDependency(videoInputNode,'input');
videoOutputNode.setDependency(watermarkNode,'watermark');

// create the watermark image members
watermarkNode.addMember('filePath','String','/development/watermark/dvd-logo.png');
watermarkNode.addMember('clientData','String','Client Name & Address');
watermarkNode.addMember('width','Size',0);
watermarkNode.addMember('height','Size',0);
watermarkNode.addMember('pixels','RGBA[]');

// create the input video members
videoInputNode.addMember('filePath','String','/development/watermark/oneyear.mp4');
videoInputNode.addMember('video','VideoHandle');
videoInputNode.addMember('pixels','RGB[]');
videoInputNode.addMember('time','Scalar',0.0);

// create the output video members
videoOutputNode.addMember('filePath','String','/development/watermark/oneyear.mpeg');
videoOutputNode.addMember('video','VideoHandle');
videoOutputNode.addMember('offsetWidth','Scalar',0.0);
videoOutputNode.addMember('offsetHeight','Scalar',0.0);
videoOutputNode.addMember('directionWidth','Scalar',1.0);
videoOutputNode.addMember('directionHeight','Scalar',1.0);
videoOutputNode.addMember('pixels','RGB[]');

// create the watermark operator
createOperator(watermarkNode,{
  name: 'openWaterMark',
  srcCode: [
    'use FabricCIMG;',
    'use FabricFILESYSTEM;',
    '',
    'operator openWaterMark(io String filePath, io String clientText, io Size width, io Size height, io RGBA pixels[]) {',
    '  if(pixels.size() == 0) {',
    '    FabricFileHandleWrapper wrapper;',
    '    wrapper.setAbsolutePath(filePath);',
    '    FabricCIMGOpenFileHandle(wrapper.getHandle(), width, height, pixels);',
    '    report("Loaded Watermark image, resolution "+width+"x"+height+".");',
    '    ',
    '    Size textWidth;',
    '    Size textHeight;',
    '    RGBA textPixels[];',
    '    FabricCIMGCreateFromText(clientText,textWidth,textHeight,textPixels);',
    '    report("Created client text image, resolution "+textWidth+"x"+textHeight+".");',
    '    Size centerWidth = (width - textWidth) * 0.5;',
    '    Size centerHeight = 190;',
    '    ',
    '    if (textWidth<width && textHeight<height) {',
    '      for(Size textX=0;textX<textWidth;textX++) {',
    '        Size x = centerWidth + textX;',
    '        for(Size textY=0;textY<textHeight;textY++) {',
    '          Size y = centerHeight + textY;',
    '          Size index = y * width + x;',
    '          Size textIndex = textY * textWidth + textX;',
    '          pixels[index] = textPixels[textIndex];',
    '        }',
    '      }',
    '      report("Copied client text into watermark image.");',
    '    }',
    '    else {',
    '      report("Watermark image too small to copy client text!");',
    '    }',
    '  }',
    '}'
  ],
  binding: [
    'self.filePath',
    'self.clientData',
    'self.width',
    'self.height',
    'self.pixels'
  ]
});

// create the open video operator
createOperator(videoInputNode,{
  name: 'openVideoFile',
  srcCode: [
    'use FabricVIDEO;',
    'use FabricFILESYSTEM;',
    '',
    'operator openVideoFile(io String filePath, io VideoHandle video) {',
    '  if(!video.pointer) {',
    '    FabricFileHandleWrapper wrapper;',
    '    wrapper.setAbsolutePath(filePath);',
    '    FabricVIDEOOpenFileHandle(wrapper.getHandle(), video);',
    '    report("Loaded video handle, resolution "+video.width+"x"+video.height+".");',
    '  }',
    '}'
  ],
  binding: [
    'self.filePath',
    'self.video'
  ]
});

// create the seek video operator
createOperator(videoInputNode,{
  name: 'seekVideoFile',
  srcCode: [
    'use FabricVIDEO;',
    '',
    'operator seekVideoFile(io VideoHandle video, io RGB pixels[], io Scalar time) {',
    '  if(video.pointer) {',
    '    FabricVIDEOSeekTime(video,time);',
    '    FabricVIDEOGetAllPixels(video,pixels);',
    '  }',
    '}'
  ],
  binding: [
    'self.video',
    'self.pixels',
    'self.time'
  ]
});

// create the output video create operator
createOperator(videoOutputNode,{
  name: 'createVideoOutput',
  srcCode: [
    'use FabricVIDEO;',
    'use FabricFILESYSTEM;',
    '',
    'operator createVideoOutput(io VideoHandle video, io String filePath, io VideoHandle input) {',
    '  if(!video.pointer) {',
    '    FabricFileHandleWrapper wrapper;',
    '    wrapper.setAbsolutePath(filePath);',
    '    FabricVIDEOCreateFromFileHandle(wrapper.getHandle(), input.width, input.height, video);',
    '    report("Created output video handle, resolution "+video.width+"x"+video.height+".");',
    '  }',
    '}'
  ],
  binding: [
    'self.video',
    'self.filePath',
    'input.video'
  ]
});

// create the compositing operator  
createOperator(videoOutputNode,{
  name: 'compositeWatermark',
  srcCode: [
    'struct sharedDataType {',
    '  Size width;',
    '  Size height;',
    '  RGBA markPixels[];',
    '  Size markWidth;',
    '  Size markHeight;',
    '  Size offsetWidth;',
    '  Size offsetHeight;',
    '};',
    '',
    'function Byte clampByte(Scalar s) {',
    '  if(s < 0.0) return Byte(0);',
    '  if(s > 255.0) return Byte(255);',
    '  return Byte(s);',
    '}',
    '',
    'operator transformPixel(io RGB pixel, Size index, Size count, sharedDataType sharedData) {',
    '  Size x = index % sharedData.width;',
    '  Size y = (index - x) / sharedData.width;',
    '  if(x < sharedData.offsetWidth || y < sharedData.offsetHeight)',
    '     return;',
    '  x -= sharedData.offsetWidth;',
    '  y -= sharedData.offsetHeight;',
    '  if(x >= sharedData.markWidth || y >= sharedData.markHeight)',
    '    return;',
    '  Size markIndex = x + y * sharedData.markWidth;',
    '  Scalar alpha = Scalar(sharedData.markPixels[markIndex].r);',
    '  pixel.r = clampByte(Scalar(pixel.r) + alpha * 0.33);',
    '  pixel.g = clampByte(Scalar(pixel.g) + alpha * 0.33);',
    '  pixel.b = clampByte(Scalar(pixel.b) + alpha * 0.33);',
    '}',
    '',
    'operator compositeWatermark(',
    '  io VideoHandle outputVideo,',
    '  io VideoHandle inputVideo,',
    '  io RGB inputPixels[],',
    '  io RGB outputPixels[],',
    '  io RGBA waterMarkPixels[],',
    '  io Size waterMarkWidth,',
    '  io Size waterMarkHeight,',
    '  io Scalar offsetWidth,',
    '  io Scalar offsetHeight,',
    '  io Scalar directionWidth,',
    '  io Scalar directionHeight',
    ') {',
    '  if(!inputVideo.pointer || !outputVideo.pointer || !waterMarkPixels.size())',
    '    return;',
    '',
    '  offsetWidth += directionWidth;',
    '  offsetHeight += directionHeight;',
    '  if(offsetWidth < 0.0) {',
    '    directionWidth *= -1.0;',
    '    offsetWidth = 0.0;',
    '  } else if(Size(offsetWidth) >= inputVideo.width - waterMarkWidth) {',
    '    directionWidth *= -1.0;',
    '    offsetWidth = Scalar(inputVideo.width - waterMarkWidth - 1);',
    '  }',
    '  if(offsetHeight < 0.0) {',
    '    directionHeight *= -1.0;',
    '    offsetHeight = 0.0;',
    '  } else if(Size(offsetHeight) >= inputVideo.height - waterMarkHeight) {',
    '    directionHeight *= -1.0;',
    '    offsetHeight = Scalar(inputVideo.height - waterMarkHeight - 1);',
    '  }',
    '',
    '  sharedDataType sharedData;',
    '  sharedData.width = inputVideo.width;',
    '  sharedData.height = inputVideo.height;',
    '  sharedData.markPixels = waterMarkPixels;',
    '  sharedData.markWidth = waterMarkWidth;',
    '  sharedData.markHeight = waterMarkHeight;',
    '  sharedData.offsetWidth = Size(offsetWidth);',
    '  sharedData.offsetHeight = Size(offsetHeight);',
    '  ValueProducer<sharedDataType> sharedDataProducer = createConstValue(sharedData);',
    '',
    '  ArrayProducer<RGB> pixelsProducer = createConstArray(inputPixels);',
    '  ArrayProducer<RGB> compositeProducer = createArrayTransform(pixelsProducer, transformPixel,sharedDataProducer);',
    '  outputPixels = compositeProducer.produce();',
    '}'
  ],
  binding: [
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
});

// create the output video add frame operator
createOperator(videoOutputNode,{
  name: 'writeVideoFrame',
  srcCode: [
    'use FabricVIDEO;',
    '',
    'operator writeVideoFrame(io VideoHandle video, io RGB pixels[], io Scalar time) {',
    '  if(video.pointer) {',
    '    FabricVIDEOWriteAllPixels(video,pixels);',
    '  }',
    '}'
  ],
  binding: [
    'self.video',
    'self.pixels',
    'input.time'
  ]
});

// evaluate the nodes!
videoOutputNode.evaluate();

var videoData = videoInputNode.getData('video',0);
var fps = videoData.fps;
var duration = videoData.duration;
var time = 0.0;


// output some standard information
console.log('--------------------------------------');
console.log('Input video: '+videoInputNode.getData('filePath',0));
console.log('WaterMark: '+watermarkNode.getData('filePath',0));
console.log('Output video: '+videoOutputNode.getData('filePath',0));
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
    process.stdout.write('\r --> Converting: ['+prog+'] '+logTime+'%.');
    lastLogTime = logTime;
  }
}
logProgress();

// increment the time, to render each frame one after another
var incrementTime = function() {
  if(time >= duration) {
    logProgress();
    F.close();
    console.log('\ndone.')
    return;
  }
  
  setTimeout(function(){
    time += 1.0 / fps;
    videoInputNode.setData('time',0,time);
    videoOutputNode.evaluate();
    logProgress();
    incrementTime();
  }, 1);
};
incrementTime();
