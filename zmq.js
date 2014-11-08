var zerorpc = require("zerorpc");
var camxes = require('./camxes.js');
var camxes_exp = require('./camxes-exp.js');
var camxes_pre = require('./camxes_preproc.js');
var camxes_post = require('./camxes_postproc.js');

var server = new zerorpc.Server({
    parse: function(text, grammar, rule, mode, reply) {
        reply(null, run_camxes(text, grammar, rule, mode), false);
    }
});

function run_camxes(input, grammar, rule, mode) {
	var result;
  console.log('parsing ' + input);
	result = camxes_pre.preprocessing(input);
	try {
    if (grammar == "exp") {
      result = camxes_exp.parse(result, rule);
    } else {
      result = camxes.parse(result, rule);
    }
	} catch (e) {
		return e;
	}
  return camxes_post.postprocessing(JSON.stringify(result, undefined, 2), mode);
}

server.bind("tcp://0.0.0.0:4242");

