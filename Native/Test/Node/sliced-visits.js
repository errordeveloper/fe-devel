FABRIC = require('Fabric').createClient();

parentOp = FABRIC.DependencyGraph.createOperator( "parentOp" );
parentOp.setEntryFunctionName('entry');
parentOp.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 2 * input; }");

parentBinding = FABRIC.DependencyGraph.createBinding();
parentBinding.setOperator( parentOp );
parentBinding.setParameterLayout( [ "self.input", "self.output" ] );

parentNode = FABRIC.DependencyGraph.createNode( "parent" );
parentNode.addMember( "input", "Scalar" );
parentNode.addMember( "output", "Scalar" );
parentNode.resize( 2 );
parentNode.setData( 'input', 0, 3 );
parentNode.setData( 'input', 1, 7 );
parentNode.bindings.append( parentBinding );

childOp = FABRIC.DependencyGraph.createOperator( "childOp" );
childOp.setEntryFunctionName('entry');
childOp.setSourceCode("operator entry( io Scalar input<>, Size index, io Scalar output ) { output = 2 * input[index]; }");

childBinding = FABRIC.DependencyGraph.createBinding();
childBinding.setOperator( childOp );
childBinding.setParameterLayout( [ "parent.output<>", "self.index", "self.output" ] );

childNode = FABRIC.DependencyGraph.createNode( "child" );
childNode.setDependency( parentNode, "parent" );
childNode.addMember( "output", "Scalar" );
childNode.resize( 2 );
childNode.bindings.append( childBinding );
if (childNode.getErrors().length)
  console.log(childNode.getErrors());

parentEHPreOp = FABRIC.DependencyGraph.createOperator( "parentEHPreOp" );
parentEHPreOp.setEntryFunctionName('entry');
parentEHPreOp.setSourceCode("operator entry( Size index, io Scalar output ) { report 'parentEHPreOp begin'; report '  Index: ' + index; report '  Output: ' +output; report 'parentEHPreOp end'; }");

parentEHPreBinding = FABRIC.DependencyGraph.createBinding();
parentEHPreBinding.setOperator( parentEHPreOp );
parentEHPreBinding.setParameterLayout( [ "node.index", "node.output" ] );

parentEHPostOp = FABRIC.DependencyGraph.createOperator( "parentEHPostOp" );
parentEHPostOp.setEntryFunctionName('entry');
parentEHPostOp.setSourceCode("operator entry( Size index, io Scalar output ) { report 'parentEHPostOp begin'; report '  Index: ' + index; report '  Output: ' +output; report 'parentEHPostOp end'; }");

parentEHPostBinding = FABRIC.DependencyGraph.createBinding();
parentEHPostBinding.setOperator( parentEHPostOp );
parentEHPostBinding.setParameterLayout( [ "node.index", "node.output" ] );

parentEH = FABRIC.DependencyGraph.createEventHandler( "parentEH" );
parentEH.setScope( 'node', parentNode );
parentEH.preDescendBindings.append( parentEHPreBinding );
parentEH.postDescendBindings.append( parentEHPostBinding );

childEHPreOp = FABRIC.DependencyGraph.createOperator( "childEHPreOp" );
childEHPreOp.setEntryFunctionName('entry');
childEHPreOp.setSourceCode("operator entry( Size childIndex, io Scalar childOutput, Size parentIndex, io Scalar parentOutput ) { report 'childEHPreOp begin'; report '  Child Index: ' + childIndex; report '  Child Output: ' + childOutput; report '  Parent Index: ' + parentIndex; report '  Parent Output: ' + parentOutput; report 'childEHPreOp end'; }");

childEHPreBinding = FABRIC.DependencyGraph.createBinding();
childEHPreBinding.setOperator( childEHPreOp );
childEHPreBinding.setParameterLayout( [ "child.index", "child.output", "parent.index", "parent.output" ] );

childEHPostOp = FABRIC.DependencyGraph.createOperator( "childEHPostOp" );
childEHPostOp.setEntryFunctionName('entry');
childEHPostOp.setSourceCode("operator entry( Size childIndex, io Scalar childOutput, Size parentIndex, io Scalar parentOutput ) { report 'childEHPostOp begin'; report '  Child Index: ' + childIndex; report '  Child Output: ' + childOutput; report '  Parent Index: ' + parentIndex; report '  Parent Output: ' + parentOutput; report 'childEHPostOp end'; }");

childEHPostBinding = FABRIC.DependencyGraph.createBinding();
childEHPostBinding.setOperator( childEHPostOp );
childEHPostBinding.setParameterLayout( [ "child.index", "child.output", "parent.index", "parent.output" ] );

childEH = FABRIC.DependencyGraph.createEventHandler( "childEH" );
childEH.appendChildEventHandler( parentEH );
childEH.setScope( 'parent', parentNode );
childEH.setScope( 'child', childNode );
childEH.preDescendBindings.append( childEHPreBinding );
childEH.postDescendBindings.append( childEHPostBinding );

event = FABRIC.DependencyGraph.createEvent( "event" );
event.appendEventHandler( childEH );
event.fire();

FABRIC.close();
