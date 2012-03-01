/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FC = require('Fabric').createClient();
KLC = FC.KLC.createCompilation("foo.kl", "operator foo() { bad }");
console.log(KLC.getSources());
KLC.addSource("bar.kl", "operator bar() {}");
console.log(KLC.getSources());
var KLE = KLC.run();
console.log(KLE.getDiagnostics());
KLC.removeSource("foo.kl");
console.log(KLC.getSources());
KLE = KLC.run();
console.log(KLE.getDiagnostics());
FC.flush();
FC.close();
