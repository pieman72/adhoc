var Adhoc = Adhoc || {};

Adhoc.print = function(){
	var f = (typeof(console)===undefined) ? alert : console.log;
	for(var i=0; i<arguments.length; ++i) f(arguments[i]);
}
