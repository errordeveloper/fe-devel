F = require('Fabric').createClient();

var node = F.DG.createNode("foo");
node.addMember("path", "String");
node.setData("path", "./test.txt");

var op = F.DG.createOperator("writeReadStringArray");
op.setSourceCode(
  'use FabricFILESTREAM;\n'+
  'use FabricFileStream;\n'+
  'use FabricFILESYSTEM;\n'+
  '\n'+
  'operator writeReadStringArray(\n'+
  ' io String path,\n'+
  '){\n'+
  '\n'+
  ' FabricFileHandleWrapper wrapper;\n'+
  ' wrapper.setAbsolutePath(path);\n'+
  ' FabricFileStream writestream;\n'+
  ' writestream.open(wrapper.getHandle(),"w");\n'+
  '\n'+
  ' if(writestream.isValid() ) {\n'+
  '   String dataNames[];\n'+
  '   for(Integer i=0; i<10; i++){\n'+
  '     dataNames.push(\'Bob\');\n'+
  '     dataNames.push(\'Fred\');\n'+
  '     dataNames.push(\'Sue\');\n'+
  '     dataNames.push(\'steve\');\n'+
  '   }\n'+
  '   writestream.writeStringArray(dataNames);\n'+
  '\n'+
  '   writestream.close();\n'+
  ' }\n'+
  '\n'+
  ' FabricFileStream readstream;\n'+
  ' readstream.open(wrapper.getHandle(),"r");\n'+
  ' if(readstream.isValid()) {\n'+
  '   String dataNames[];\n'+
  '   readstream.readStringArray(dataNames);\n'+
  '   report(dataNames);\n'+
  ' }\n'+
  '}'
);


op.setEntryFunctionName('writeReadStringArray');
if (op.getErrors().length > 0) {
  if (op.getDiagnostics().length > 0)
    console.log(JSON.stringify(op.getDiagnostics()));
}

var bind = F.DG.createBinding();
bind.setOperator(op);
bind.setParameterLayout([
  'self.path'
]);

node.bindings.append(bind);
if (node.getErrors().length > 0)
  console.log(JSON.stringify(node.getErrors()));

node.evaluate();

F.close();

require('fs').unlinkSync('test.txt');

