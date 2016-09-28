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
	    // STAVPlayer.play('rtsps://52.43.12.189:443/vzmodulelive/4CD161S007D49_1475088034637?egressToken=7d320397_38f4_450d_83bc_00b700a731a7&userAgent=Android&cameraId=4CD161S007D49_1475088034637&cafile=/data/user/0/com.netgear.android/files/wowza.netgear.com.crt',
            //                 0.5, 'wowza.netgear.com.crt');
            STAVPlayer.play('rtsp://admin:password@192.168.40.152/profile4/media.smp', 0.5, '');
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
