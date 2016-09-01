"use strict";

// console.log does not work properly in Tizen IDE for NaCl projects
var console = {};
console.log = function(msg) {
    var logElem = document.getElementById('log');
    logElem.value += msg.trim() + '\n';
    logElem.scrollTop = logElem.scrollHeight;
}

// STAVPlayer usage example
window.onload = function() {
    STAVPlayer.module = document.getElementById('stavplayer');
    STAVPlayer.addListeners(document.getElementById('listener'));
    STAVPlayer.handleBufferingCompleted = function(e) {
        // update UI state
    }

    document.addEventListener('keydown', function(e) {
        switch(e.keyCode) {
        case 13:
            console.log('keydown: ENTER');
            STAVPlayer.play('rtsp://admin:password@192.168.42.32/profile4/media.smp', 0.5);  //2nd arg - callback frequence 'in sec' of audio_level
            // STAVPlayer.play('rtsp://ec2-52-42-163-84.us-west-2.compute.amazonaws.com:8554/profile3');
            break;
        case 37:
            console.log('keydown: LEFT arrow (Stop)');
            STAVPlayer.stop();
            break;
        case 38:
            console.log('keydown: UP arrow (Mute)');
            STAVPlayer.mute();
            break;
        case 39:            
            console.log('keydown: RIGHT arrow');
            break;
        case 40:
            console.log('keydown: DOWN arrow');
            break;
        }
    });
}
