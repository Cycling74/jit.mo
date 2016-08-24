autowatch = 1;

var ctx = "w";
var dim = 40;
var bound = 100;

var agents = new Array();

// for the anim.drive rotateto message
var e2q = new JitterObject("jit.euler2quat");
var quat = new JitterObject("jit.quat");
var xmat = new JitterMatrix(1, "float32", dim);
var sinmat = new JitterMatrix(1, "float32", dim);
var linemat = new JitterMatrix(1, "float32", dim);
var joinmat = new JitterMatrix(3, "float32", dim);

function Agent(ctxname) {
	this.mesh = new JitterObject("jit.gl.mesh", ctxname);
	this.mesh.draw_mode = "line_strip";
	this.mesh.color = [Math.random(),Math.random(),Math.random()];
	this.mesh.antialias = 1;
	this.mesh.line_width = Math.random()*3.0 + 1.0;

	this.an = new JitterObject("jit.anim.node");
	this.mesh.anim = this.an.name;
	this.an.movemode = "local";

	this.ad = new JitterObject("jit.anim.drive");
	this.ad.targetname = this.an.name;
	this.ad.easefunc = "cubic";

	this.line = new JitterObject("jit.mo.func");
	this.line.functype = "line";
	this.sin = new JitterObject("jit.mo.func");
	this.sin.functype = "sin";

	this.join = new JitterObject("jit.mo.join");
	this.join.dim = 100;

	this.join.drawto = ctxname;
	this.line.join = this.join.name;
	this.sin.join = this.join.name;
    
    this.speed = 1.0;
    this.damp = 0;

	this.initagent = function() {
		// create random orientation and move along Z axis
		this.mesh.position = [0,0,0];
		var xrot = (Math.random() * 60 + 30) * -1.;
		var yrot = Math.random() * 180 - 90.;
		var rot = Math.random() * 360. - 180.;
		//this.mesh.rotatexyz = [xrot, yrot, 0];
		this.mesh.rotatexyz = [rot, rot, rot];
		this.speed = Math.random();
		this.ad.move(0, 0, this.speed);	

		this.sin.freq = this.speed * 5.0 + 0.25;
		this.sin.speed = this.speed*2.0;
		this.line.scale = 1.0 + this.sin.freq;

	}; 

    this.free = function() {
    	this.mesh.freepeer();
    	this.an.freepeer();
    	this.ad.freepeer();
    	this.line.freepeer();
    	this.sin.freepeer();
    	this.join.freepeer();
    }

    this.update = function() {
    	this.sin.matrixcalc(sinmat, sinmat);
    	this.line.matrixcalc(linemat, linemat);
    	this.join.matrixcalc([xmat, sinmat, linemat], joinmat);

    	this.mesh.jit_matrix(joinmat.name);

		// every frame check if agent is out of bounds
		var pos = this.an.position;
		if(Math.abs(pos[0])>bound||Math.abs(pos[1])>bound||Math.abs(pos[2])>bound) {
			this.an.position=[0,0,0];
			/*if(this.damp > 10) {
				quat.quat1 = this.an.quat;
				this.an.quat = quat.inverse;
				this.damp = 0;
			}
			else {
				this.damp ++;
			}*/
		}

		// randomly turn some agents
		if(Math.random() > 0.9 && this.speed > 0.75) {
			e2q.euler = [Math.random()*360,Math.random()*360,0];
			var rtoargs = e2q.quat;
			rtoargs[4] = Math.random()+0.5;
			this.ad.rotateto(rtoargs);
		}
    };    
}

function init(count) {
	for(var ag in agents) {
		agents[ag].free();
	}
	agents = new Array();

	for(var i = 0; i < count; i++) {
		var ag = new Agent(ctx);
		ag.initagent();
		agents.push(ag);
	}
}

function bang() {
	for(var ag in agents) {
		agents[ag].update();
	}
}


function reset() {
	for(var ag in agents) {
		agents[ag].initagent();
	}
}
