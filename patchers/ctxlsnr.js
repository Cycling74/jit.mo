// create our listener object for our window
var mylistener = new JitterListener("ctx",callbackfun);
var prevts = 0;

var phase = 0.
var loop = 1;
var inval = 0.;
var outval = 2.;
var isplaying = 0;
var speed = 1.0;

Math.fmod = function (a,b) { return Number((a - (Math.floor(a / b) * b)).toPrecision(8)); };

function callbackfun(event)
{
	if (event.eventname=="swap" && isplaying) {
		var delta = 0.0;
		var curts = new Date().getTime();
		//post("cur ts: " +curts+"\n");
		if(prevts!=0)
			delta = curts - prevts;
		prevts = curts;
		dophase(delta*0.001);
		//post("delta: "+delta+"\n");
	}
}
callbackfun.local = 1;

function dophase(delta) {
	phase = phase + (delta * speed * 2.0); 
	if(loop)
		phase = Math.fmod(phase, 2.0);
	else if(phase > outval) {
		phase = outval;
		isplaying = 0;
	}
	outlet(0, phase);
}

function setrange (ival, oval) {
	inval = ival;
	outval = oval;
}

function setspeed(s) {
	speed = s;
}

function setloop(l) {
	loop = l;
}

function start() {
	phase = inval;
	prevts = 0;
	isplaying = 1;
}

function stop() {
	isplaying = 0;
}