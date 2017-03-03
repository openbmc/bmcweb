angular.module('bmcApp').controller('KvmController', function($scope, $location) {


    /*jslint white: false */
    /*global window, $, Util, RFB, */
    "use strict";
    var INCLUDE_URI = "noVNC/"
    // Load supporting scripts
    Util.load_scripts(["webutil.js", "base64.js", "websock.js", "des.js",
                        "keysymdef.js", "xtscancodes.js", "keyboard.js",
                        "input.js", "display.js", "inflator.js", "rfb.js",
                        "keysym.js"]);

    var rfb;
    var resizeTimeout;


    function UIresize() {
        if (WebUtil.getConfigVar('resize', false)) {
            var innerW = window.innerWidth;
            var innerH = window.innerHeight;
            var controlbarH = $D('noVNC_status_bar').offsetHeight;
            var padding = 5;
            if (innerW !== undefined && innerH !== undefined)
                rfb.requestDesktopSize(innerW, innerH - controlbarH - padding);
        }
    }
    function FBUComplete(rfb, fbu) {
        UIresize();
        rfb.set_onFBUComplete(function() { });
    }
    function sendCtrlAltDel() {
        rfb.sendCtrlAltDel();
        return false;
    }
    function xvpShutdown() {
        rfb.xvpShutdown();
        return false;
    }
    function xvpReboot() {
        rfb.xvpReboot();
        return false;
    }
    function xvpReset() {
        rfb.xvpReset();
        return false;
    }
    function updateState(rfb, state, oldstate, msg) {
        var s, sb, cad, level;
        s = $D('noVNC_status');
        sb = $D('noVNC_status_bar');
        cad = $D('sendCtrlAltDelButton');
        switch (state) {
            case 'failed':       level = "error";  break;
            case 'fatal':        level = "error";  break;
            case 'normal':       level = "normal"; break;
            case 'disconnected': level = "normal"; break;
            case 'loaded':       level = "normal"; break;
            default:             level = "warn";   break;
        }

        if (state === "normal") {
            cad.disabled = false;
        } else {
            cad.disabled = true;
            xvpInit(0);
        }

        if (typeof(msg) !== 'undefined') {
            sb.setAttribute("class", "noVNC_status_" + level);
            s.innerHTML = msg;
        }
    }

    window.onresize = function () {
        // When the window has been resized, wait until the size remains
        // the same for 0.5 seconds before sending the request for changing
        // the resolution of the session
        clearTimeout(resizeTimeout);
        resizeTimeout = setTimeout(function(){
            UIresize();
        }, 500);
    };

    function xvpInit(ver) {
        var xvpbuttons;
        xvpbuttons = $D('noVNC_xvp_buttons');
        if (ver >= 1) {
            xvpbuttons.style.display = 'inline';
        } else {
            xvpbuttons.style.display = 'none';
        }
    }

    window.onscriptsload = function () {
        var host, port, password, path, token;

        $D('sendCtrlAltDelButton').style.display = "inline";
        $D('sendCtrlAltDelButton').onclick = sendCtrlAltDel;
        $D('xvpShutdownButton').onclick = xvpShutdown;
        $D('xvpRebootButton').onclick = xvpReboot;
        $D('xvpResetButton').onclick = xvpReset;

        host = $location.host();
        port = 9000;
        password = "";
        token = "1234";
        path = "/";

        try {
            rfb = new RFB({'target':       $D('noVNC_canvas'),
                            'encrypt':      true,
                            'local_cursor': true,
                            'onUpdateState':  updateState,
                            'onXvpInit':    xvpInit,
                            'onFBUComplete': FBUComplete});
        } catch (exc) {
            updateState(null, 'fatal', null, 'Unable to create RFB client -- ' + exc);
            return; // don't continue trying to connect
        }

        rfb.connect(host, port, password, path);
    };
});