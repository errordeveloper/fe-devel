// Creating the FABRIC object
FABRIC = require('Fabric').createClient()

// Registered types
Vec3 = function(x, y, z) {
  if (typeof x === 'number' && typeof y === 'number' && typeof z === 'number') {
    this.x = x;
    this.y = y;
    this.z = z;
  }
  else if (x === undefined && y === undefined && z === undefined) {
    this.x = 0;
    this.y = 0;
    this.z = 0;
  }
  else throw'new Vec3: invalid arguments';
};
vec3KLBindings = "\n\
// Construct a Vec3 from three Scalars\n\
function Vec3(Scalar x, Scalar y, Scalar z) {\n\
  this.x = x;\n\
  this.y = y;\n\
  this.z = z;\n\
}\n\
\n\
// Add two Vec3s\n\
function Vec3 + (Vec3 a, Vec3 b) {\n\
  return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);\n\
}\n\
";
FABRIC.RT.registerType('Vec3', {
  members: [{x: 'Scalar'}, {y: 'Scalar'}, {z: 'Scalar'}],
  constructor: Vec3,
  klBindings: {
    filename: "inline",
    sourceCode: vec3KLBindings
  }
});
FABRIC.RT.getRegisteredTypes().Vec3;

// Operator creation
op = FABRIC.DG.createOperator( "doSomething" );

// Setting operator source code
op.setSourceCode("operator entry( io Scalar result, in Size index, in Container self ) { result = 3.14 }");
op.getDiagnostics();
op.setSourceCode("operator entry( io Scalar result, in Size index, in Conatiner self ) { result = 3.14; }");
op.getDiagnostics();

// Setting the operator entry point
op.setEntryFunctionName('entry');
op.getEntryFunctionName();

// Node creation
node = FABRIC.DG.createNode( "vertices" );

// Getting a node's name
node.getName();

// Adding and getting node members
node.addMember("position", "Vec3", new Vec3(0, 0, 0));
node.getMembers().position;

// getData and setData
node.getData('position', 0);
node.setData('position', 0, new Vec3(1, 2, 3));
node.getData('position', 0);

// Node slice counts
node.getCount();
node.getData('position', 1);
node.setCount(2);
node.getCount();
node.getData('position', 1);

// Node dependencies
anotherNode = FABRIC.DG.createNode( "originalVertices" );
node.setDependency( anotherNode, "original" );
node.getDependencies();

// Node evaluation
op = FABRIC.DG.createOperator( "offsetPosition" );
op.setEntryFunctionName( "offset" );
op.setSourceCode( "require Vec3; operator offset( io Vec3 position, io Vec3 newPosition ) { newPosition = position + Vec3(1.0,1.0,1.0); }" );
binding = FABRIC.DG.createBinding();
binding.setOperator(op);
binding.setParameterLayout(["self.position","self.newPosition"]);
node.addMember( "newPosition", "Vec3" );
node.bindings.append( binding );
node.getData( "position", 0 );
node.getData( "newPosition", 0 );
node.evaluate();
node.getData( "newPosition", 0 );

// Event creation
event = FABRIC.DG.createEvent("anEvent");
event.getName();

// Creating EventHandlers and appending them to Events
eventHandler = FABRIC.DG.createEventHandler("trivialEventHandler");
event.appendEventHandler(eventHandler);
event.getEventHandlers();

// Operators and EventHandlers
op = FABRIC.DG.createOperator("trivialOperator");
op.setSourceCode("operator entry() { report 'Ran trivialOperator'; }");
op.setEntryFunctionName('entry');
binding = FABRIC.DG.createBinding();
binding.setOperator(op);
binding.setParameterLayout([]);
eventHandler.preDescendBindings.append(binding);
event.fire();
anotherOp = FABRIC.DG.createOperator("trivialOperatorTwo");
anotherOp.setSourceCode("operator entry() { report 'Ran trivialOperatorTwo'; }");
anotherOp.setEntryFunctionName('entry');
binding = FABRIC.DG.createBinding();
binding.setOperator(anotherOp);
binding.setParameterLayout([]);
eventHandler.postDescendBindings.append(binding);
event.fire();

// Child EventHandlers
binding = FABRIC.DG.createBinding();
binding.setOperator(op);
binding.setParameterLayout([]);
ceh = FABRIC.DG.createEventHandler("childEventHandler");
ceh.preDescendBindings.append(binding);
eventHandler.appendChildEventHandler(ceh);
event.fire();

// The setScope method
node = FABRIC.DG.createNode("someNode");
node.addMember( "x", "Scalar" );
node.addMember( "y", "Scalar" );
squareOp = FABRIC.DG.createOperator("squareOp");
squareOp.setSourceCode("operator entry( io Scalar x, io Scalar y ) { y = x * x; }");
squareOp.setEntryFunctionName("entry");
binding = FABRIC.DG.createBinding();
binding.setOperator(squareOp);
binding.setParameterLayout(['self.x','self.y']);
node.bindings.append(binding);
displayOp = FABRIC.DG.createOperator("displayOp");
displayOp.setSourceCode( "operator entry( io Scalar x, io Scalar y ) { report x + ' squared is ' + y; }" );
displayOp.setEntryFunctionName("entry");
binding = FABRIC.DG.createBinding();
binding.setOperator(displayOp);
binding.setParameterLayout(['mynode.x','mynode.y']);
eventHandler.setScope("mynode",node);
eventHandler.postDescendBindings.append(binding);
event.fire();
node.setData('x',5.0);
event.fire();

// EventHandler data 
eventHandler.setScopeName("childEventHandler");
eventHandler.addMember("x","Scalar");
eventHandler.addMember("y","Scalar");
binding = FABRIC.DG.createBinding();
binding.setOperator(squareOp);
binding.setParameterLayout(['childEventHandler.x','childEventHandler.y']);
ceh.preDescendBindings.append(binding);
binding = FABRIC.DG.createBinding();
binding.setOperator(displayOp);
binding.setParameterLayout(['childEventHandler.x','childEventHandler.y']);
ceh.preDescendBindings.append(binding);
event.fire();
eventHandler.setData("x",7.31);
event.fire();
