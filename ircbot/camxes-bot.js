var irc = require('irc');
var fs = require('fs');
var yaml = require('js-yaml');
var Entities = require('html-entities').AllHtmlEntities;
var twitter = require('immortal-ntwitter');
var rss = require('rss-watcher');
var bitly = require('node-bitlyapi');
var camxes = require('../camxes.js');
var camxes_exp = require('../camxes-exp.js');
var camxes_pre = require('../camxes_preproc.js');
var camxes_post = require('../camxes_postproc.js');

var config = {
  server: 'irc.freenode.net',
  nick: ['camxes', 'nuzba'],
  options: {
    channels: ['#lojban', '#ckule', '#balningau'],
    debug: false
  },
  nuzba: {
    delay: 120,
    secrets: __dirname + '/nuzba.secrets',
    log: __dirname + '/nuzba.log',
    links: {
      setup: init_bitly,
      call: get_bitly_link,
      options: {}
    },
    sources: {
      twitter: {
        setup: start_twitter_stream,
        check: null,
        options: {
          words: ['lojban', 'jbobau'],
          exclude: ['uitki', 'hettgutt'],
          retweets: false
        }
      },
      jbovlaste: {
        setup: start_jbovlaste_rss,
        check: null,
        options: {}
      },
      wiki: {
        setup: null,
        check: null,
        options: {}
      }
    }
  }
};

function init_bitly(options) {
    var bit = new bitly({
        client_id:     options.secrets.client.id,
        client_secret: options.secrets.client.secret
    });
    bit.authenticate(options.secrets.username, options.secrets.password, function(err, access_token) {
        if (err) throw err;
        bit.setAccessToken(access_token);
        console.log('logged into bit.ly!');
    });
    return bit;
}

function get_bitly_link(bit, url, options, next) {
    bit.shortenLink(url, function (err, result) {
        if (err) throw err;
        console.log('bitly says: ' + result);
        return next(JSON.parse(result).data.url);
    });
}

function start_twitter_stream(options) {
    twit = twitter.create({
        consumer_key:        options.secrets.consumer.key,
        consumer_secret:     options.secrets.consumer.secret,
        access_token_key:    options.secrets.access_token.key,
        access_token_secret: options.secrets.access_token.secret
    });

    console.log('logged into twitter!');

    twit.immortalStream('statuses/filter', { 'track': options.words.join(',') }, function (stream) {
        entities = new Entities();
        stream.on('data', function (data) {
            if ((options.exclude.indexOf(data.user.screen_name) == -1) && (options.retweets || !data.retweeted_status)) {
                console.log('captured tweet from @' + data.user.screen_name + " {" + data.text + "}");
                options.link('https://twitter.com/' + data.user.id_str + '/status/' + data.id_str, function (url) {
                    options.enqueue('@' + data.user.screen_name
                                        + ': ' + entities.decode(data.text.replace(/\n/g, '\\n'))
                                        + ' [' + url + ']');
                });
            }
        });
    });

    return twit;
}

function start_jbovlaste_rss(options) {
  var watcher = new rss('http://jbovlaste.lojban.org/recent.rss');
  watcher.on('new article', console.log);
  //watcher.set({interval: 30});
  //watcher.run(function(err, articles) { if (err) throw err; });
  return watcher;
}

var news_timer = null;
var news_queue = [];
fs.exists(config.nuzba.log, function (exists) {
    if (exists) {
        fs.readFile(config.nuzba.log, function (err, data) {
            if (err) throw err;
            news_queue = JSON.parse(fs.readFileSync(config.nuzba.log));
        });
    }
});

function enqueue_news_item(item) {
    news_queue.push(item);
    fs.writeFile(config.nuzba.log, JSON.stringify(news_queue),
        function (e) {
            if (e) console.log('error writing log: ' + e);
        });
}

function dequeue_news_item() {
    if (news_queue.length > 0) console.log('start the presses!! ' + news_queue.length + ' incoming');

    for (i = 0; i < news_queue.length; i++) {
        client[1].action(config.options.channels[0], news_queue[i]);
    }

    news_queue = [];
    fs.writeFileSync(config.nuzba.log, JSON.stringify(news_queue));
    news_timer = setTimeout(dequeue_news_item, config.nuzba.delay * 1000);
}

// start up IRC clients
var client = new Array(config.nick.length);
for (var i = 0; i < config.nick.length; i++) {
    client[i] = new irc.Client(config.server, config.nick[i], config.options);

    client[i].addListener('message', (function(i) { return function(from, to, text, message) {
            processor(i, client, from, to, text, message);
    };})(i));
}

// set up nuzba stuff
var news = [];
var secrets = yaml.safeLoad(fs.readFileSync(config.nuzba.secrets, 'utf8'));
(function (links) {
    links.options.secrets = secrets.links;
    if (links.setup) links.setup = links.setup(links.options);
})(config.nuzba.links);
var links_wrapper = function (url, next) {
    config.nuzba.links.call(config.nuzba.links.setup,
                            url,
                            config.nuzba.links.options,
                            next);
}
for (var source in config.nuzba.sources) {
    (function (src, nsrc) {
        src.options.secrets = secrets[nsrc];
        src.options.enqueue = enqueue_news_item;
        src.options.link = links_wrapper;
        if (src.setup) src.setup = src.setup(src.options);
    })(config.nuzba.sources[source], source);
}

function make_regexps(nick) {
    return {
        coi:  new RegExp("(^| )coi la \\.?"  + nick + "\\.?"),
        juhi: new RegExp("(^| )ju'i la \\.?" + nick + "\\.?"),
        kihe: new RegExp("(^| )ki'e la \\.?" + nick + "\\.?")
    };
}

var processor = function(i, client, from, to, text, message) {
  if (!text) return;
  var sendTo = from; // send privately
  if (to.indexOf('#') > -1) {
    sendTo = to; // send publicly
  }
  if (sendTo == to) {  // public
    var regexps = make_regexps(config.nick[i]);
    if (i == 0 && text.indexOf(config.nick[i] + ": ") == '0') {
      text = text.substr(config.nick[i].length + 2);
      var ret = extract_mode(text);
      client[i].say(sendTo, run_camxes(ret[0], ret[1], ret[2]));
    } else if (text.search(regexps.coi) >= 0) {
      client[i].say(sendTo, "coi");
    } else if (text.search(regexps.juhi) >= 0) {
      client[i].say(sendTo, "re'i");
    } else if (text.search(regexps.kihe) >= 0) {
      client[i].say(sendTo, "je'e fi'i");
    }

    if (i == 1) { // public (nuzba)
      clearTimeout(news_timer);
      news_timer = setTimeout(dequeue_news_item, config.nuzba.delay * 1000);
    }
  } else if (i == 0) {  // private (camxes)
	var ret = extract_mode(text);
    client[i].say(sendTo, run_camxes(ret[0], ret[1], ret[2]));
  }
};

function extract_mode(input) {
  ret = [input, 2, "std"];
  flag_pattern = "[+-]\\w+"
  match = input.match(new RegExp("^\\s*((?:" + flag_pattern + ")+)(.*)"))
  if (match != null) {
    ret[0] = match[2];
    flags = match[1].match(new RegExp(flag_pattern, "g"))
    for (var i = 0; i < flags.length; ++i) {
      switch (flags[i]) {
        case "+s":
          ret[1] = ret[1] == 5 ? 6 : 3;
          break;
        case "-f":
          ret[1] = ret[1] == 3 ? 6 : 5;
          break;
        case "+exp":
        case "-std":
          ret[2] = "exp";
          break;
        case "-exp":
        case "+std":
          ret[2] = "std";
          break;
      }
    }
  }
  return ret;
}

function run_camxes(input, mode, engine) {
	var result;
	var syntax_error = false;
	result = camxes_pre.preprocessing(input);
	try {
    switch (engine) {
      case "std":
        result = camxes.parse(result);
        break;
      case "exp":
        result = camxes_exp.parse(result);
        break;
      default:
        throw "Unrecognized parser";
    }
	} catch (e) {
		result = e;
		syntax_error = true;
	}
	if (!syntax_error) {
		result = JSON.stringify(result, undefined, 2);
		result = camxes_post.postprocessing(result, mode);
	}
	return result;
}

