// jit.mo-worms creates a bunch of jit.mo structures, and corresponding 
// jit.anim position and rotation transformers. Each jit.mo output matrix 
// is combined into a single matrix and output every frame bang.

autowatch = 1;

var bounds = 100;	// bounds of our world in all directions
var dim = 40;		// dim of our jit.mo matrices
var agents = new Array();

// for the anim.drive rotateto message
var e2q = new JitterObject("jit.euler2quat");
var quat = new JitterObject("jit.quat");

// transforms jit.mo points by corresponding anim.node prior to output
var gen = new JitterObject("jit.gen");
gen.gen = "3d-transform";

// jit.mo matrixcalc matrices
var sinmat = new JitterMatrix(1, "float32", dim);
var linemat = new JitterMatrix(1, "float32", dim);
var pmat = new JitterMatrix(1, "float32", dim);
var joinmat = new JitterMatrix(3, "float32", dim);
var tmpmat = new JitterMatrix(3, "float32", dim);

// ouput matrix combines all jit.mo instances, 1 row per instance
var outputmat = null;

function Jitmo(ctxname) {
	this.line = new JitterObject("jit.mo.func");
	this.line.functype = "line";
	this.sin = new JitterObject("jit.mo.func");
	this.sin.functype = "sin";
	this.perlin = new JitterObject("jit.mo.func");
	this.perlin.functype = "perlin";	

	this.join = new JitterObject("jit.mo.join");

	this.join.drawto = ctxname;
	this.line.join = this.join.name;
	this.sin.join = this.join.name;
	this.perlin.join = this.join.name;

	this.init = function(speed) {
		this.sin.freq = speed * 2.0 + 0.25;
		this.sin.speed = speed*2.0;
		this.line.scale = 1.0 + speed * 4.0;
		this.perlin.scale = Math.random() * 0.5 + 0.75;
		this.perlin.freq = 0.25;
		this.perlin.speed = speed;
	}

    this.free = function() {
    	this.line.freepeer();
    	this.sin.freepeer();
    	this.join.freepeer();
    	this.perlin.freepeer();
    }

    this.update = function() {
    	this.sin.matrixcalc(sinmat, sinmat);
    	this.line.matrixcalc(linemat, linemat);
    	this.perlin.matrixcalc(pmat, pmat);

    	// apply both sin and perlin outputs to Y axis by adding them together
    	sinmat.op("+", sinmat, pmat);
    	this.join.matrixcalc([pmat, sinmat, linemat], joinmat);
    }
}

function Agent(ctxname, i) {
	this.index = i;
	this.an = new JitterObject("jit.anim.node");
	this.an.movemode = "local";

	this.ad = new JitterObject("jit.anim.drive");
	this.ad.targetname = this.an.name;
	this.ad.drawto = ctxname;
	this.ad.easefunc = "cubic";
    
    this.speed = 1.0;
    this.jitmo = new Jitmo(ctxname);

	this.initagent = function() {
		// we must manually update the anim.node once at init since it is not attached to a GL object
		this.an.update_node();

		// create random orientation and move along Z axis at random speed
		this.an.position = [0,0,0];
		this.an.rotatexyz = [Math.random() * 360., Math.random() * 360., Math.random() * 360.];
		this.speed = Math.random();
		this.ad.move(0, 0, this.speed);

		this.jitmo.init(this.speed);	
	}; 

    this.free = function() {
    	this.jitmo.free();

    	this.an.freepeer();
    	this.ad.freepeer();
    }

    this.rand_turn = function(max) {
		var xyz = this.an.rotatexyz;
		e2q.euler = [xyz[0] + Math.random() * max, xyz[1] + Math.random() * max, xyz[2]];
		var rtoargs = e2q.quat;
		rtoargs[4] = Math.max((1. - this.speed) * 4.0, 0.25);
		this.ad.rotateto(rtoargs);
    }

    this.update_transform = function() {
    	// update our jit.mo matrix position values by our jit.anim 3d transform
		gen.param("position", this.an.position);
		gen.param("quat", this.an.quat);
		gen.matrixcalc(joinmat, tmpmat);
	}
	
	this.update_output = function() {
		// fill the corresponding row of our output matrix
    	outputmat.dstdimstart = [0, this.index];
    	outputmat.dstdimend = [dim-1, this.index];
    	outputmat.frommatrix(tmpmat);
    }

    this.update = function(tindex) {
		// every frame check if agent is out of bounds
		var pos = this.an.position;
		if(Math.abs(pos[0]) > bounds || Math.abs(pos[1]) > bounds || Math.abs(pos[2]) > bounds) {
			this.an.position=[0,0,0];
		}

		// update our jit.mo matrices
    	this.jitmo.update();
    	
    	// transform our jit.mo matrices
    	this.update_transform();		

    	// update our output matrix
    	this.update_output();		
    };    
}

function init(ctxname, count) {
	outputmat = new JitterMatrix(3, "float32", dim, count);
	outputmat.usedstdim = 1;

	for(var a in agents) {
		agents[a].free();
	}
	agents = new Array();

	for(var i = 0; i < count; i++) {
		var ag = new Agent(ctxname, i);
		ag.initagent();
		agents.push(ag);
	}
}

function rand_turn(max_turn) {
	for(var a in agents) {
		agents[a].rand_turn(max_turn);
	}
}

function bang() {
	for(var a in agents) {
		agents[a].update(Math.floor(Math.random()*agents.length));
	}

	if(outputmat) {
		outlet(0, "jit_matrix", outputmat.name);
	}
}

function reset() {
	for(var a in agents) {
		agents[a].initagent();
	}
}
