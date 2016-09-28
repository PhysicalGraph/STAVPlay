"use strict";

var STAVPlayer = {
    module: null,
    handleBufferingComplete: null,
    playReady: false,
    MessageTo: {
        kClosePlayer: 0,
        kLoadMedia: 1,
        kPlay: 2,
        kStop: 3,
        kMute: 5,
    },
    MessageFrom: {
        kTimeUpdate: 100,
        kBufferingCompleted: 102,
        kStreamEnded: 103,
        kSetAudioLevel: 104,
        kSendStats: 105,
    },
};

STAVPlayer.moduleDidLoad = function(e) {
    console.log('module loaded');
}

STAVPlayer.moduleDidCrash = function(e) {
    console.log('module crashed ' + STAVPlayer.module.exitStatus);
}

STAVPlayer.handleMessage = function(e) {
    switch (e.data.messageFromPlayer) {
    case STAVPlayer.MessageFrom.kBufferingCompleted:
        console.log('buffering complete');
        STAVPlayer.playReady = true;
        STAVPlayer.handleBufferingCompleted(e);
        STAVPlayer.module.postMessage({'messageToPlayer': STAVPlayer.MessageTo.kPlay});
        break;
    case STAVPlayer.MessageFrom.kStreamEnded:
        console.log('stream ended');
        break;
    case STAVPlayer.MessageFrom.kTimeUpdate:
        // console.log('time: ' + e.data.time);
        break;
    case STAVPlayer.MessageFrom.kSetAudioLevel:
    	// console.log('audio level: ' + e.data.audio_level);
    	document.getElementById("audiometer").innerHTML=e.data.audio_level;
    	
        break;
    case STAVPlayer.MessageFrom.kSendStats:
        var msg = 'RTP packets_lost=' + e.data.stats_lost;
        msg    += ' jitter=' + e.data.stats_jitter;
        msg    += ' bitrate=' + e.data.stats_bitrate + ' kb/s';
        console.log(msg);
        break;
    default:
        console.log(e.data); // a log message from C code
    }    
}

STAVPlayer.play = function(url, audio_level_cb_frequency, crt_path) {
	audio_level_cb_frequency = audio_level_cb_frequency || 0;
    if (this.playReady) {
        this.module.postMessage({'messageToPlayer': this.MessageTo.kPlay});
    } else {
        this.module.postMessage({'messageToPlayer': this.MessageTo.kLoadMedia,
                                 'type' : 1, 'url': url,
                                 'audio_level_cb_frequency':audio_level_cb_frequency,
                                 'crt_path': crt_path});
    }
}

STAVPlayer.stop = function() {
    this.module.postMessage({'messageToPlayer': this.MessageTo.kStop});
}

STAVPlayer.mute = function() {
    this.module.postMessage({'messageToPlayer': this.MessageTo.kMute});
}
STAVPlayer.addListeners = function(listenerElem) {
    listenerElem.addEventListener('load', this.moduleDidLoad, true);
    listenerElem.addEventListener('message', this.handleMessage, true);
    listenerElem.addEventListener('crash', this.moduleDidCrash, true);
}
