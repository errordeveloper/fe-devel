FC = createFabricClient();
F = FC.wrapFabricClient(FC);

o = F.DG.createOperator("op");
o.setEntryFunctionName("entry");
o.setSourceCode("\
use ExceptSample;\n\
\n\
operator entry() {\n\
  report 'before test exception';\n\
  String string = 'test exception';\n\
  throw(string);\n\
  report 'after test exception';\n\
}\n\
");
if (o.getDiagnostics().length > 0 )
  printDeep(o.getDiagnostics());

binding = F.DependencyGraph.createBinding();
binding.setOperator( o );
binding.setParameterLayout([]);

eh = F.DG.createEventHandler("eh");
eh.preDescendBindings.append( binding );

e = F.DG.createEvent("e");
e.appendEventHandler( eh );
e.fire();

F.flush();
FC.dispose();
