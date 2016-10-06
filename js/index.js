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
	    STAVPlayer.play('rtsps://52.89.182.110:443/vzmodulelive/48B45B7S3C1C6_1475778807316?egressToken=cc2d6a94_5dc8_42d6_9668_3971aa29d046&userAgent=Android&cameraId=48B45B7S3C1C6_1475778807316&cafile=/data/user/0/com.netgear.android/files/wowza.netgear.com.crt',
                            0.5, 'wowza.netgear.com.crt');
            // STAVPlayer.play('rtsp://admin:password@192.168.40.152/profile4/media.smp', 0.5, '');
            // STAVPlayer.play('rtsp://ec2-52-42-163-84.us-west-2.compute.amazonaws.com:8554/profile3', 0.5, '');
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
