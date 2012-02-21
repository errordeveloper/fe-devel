/*
 *  Copyright 2010-2012 Fabric Technologies Inc. All rights reserved.
 */

F = require('Fabric').createClient();

o = F.DG.createOperator("o");
o.setEntryFunctionName("foo");
o.setSourceCode("\
operator foo( io String string )\n\
{\n\
  report string;\n\
}\n\
");

binding = F.DG.createBinding();
binding.setOperator( o );
binding.setParameterLayout( [ "peh.string" ] );

ceh = F.DG.createEventHandler("ceh");
ceh.preDescendBindings.append(binding);

peh = F.DG.createEventHandler("peh");
peh.setScopeName("peh");
peh.addMember( "string", "String" );
peh.setData( "string", "bar" );
peh.appendChildEventHandler( ceh );

e = F.DG.createEvent("e");
e.appendEventHandler( peh );

e.fire();

F.close();
