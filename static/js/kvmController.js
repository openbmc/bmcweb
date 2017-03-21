angular.module('bmcApp').controller('KvmController', 
['$scope', '$location', '$window',
function($scope, $location, $window) {


    /*jslint white: false */
    /*global window, $, Util, RFB, */
    "use strict";

    var rfb;
    var host = $location.host();
    var port = $location.port();
    var encrypt = $location.protocol() === 'https';
    var password = "";
    var token = "1234";
    var path = "kvmws";
    var target = angular.element(document.querySelector('#noVNC_canvas'))[0];
    try {
        rfb = new RFB({'target':        target,
                        'encrypt':      encrypt,
                        'local_cursor': true,
                        'onUpdateState':  updateState,
                        //'onXvpInit':    xvpInit,
                        'onFBUComplete': FBUComplete,
                        'resize': true});
        rfb.connect(host, port, password, path);
    } catch (exc) {
        updateState(null, 'fatal', null, 'Unable to create RFB client -- ' + exc);
        return; // don't continue trying to connect
    }

    

    $scope.$on("$destroy", function() {
        if (rfb) {
            rfb.disconnect();
        }
    });

    function UIresize() {
        if (WebUtil.getConfigVar('resize', false)) {
            var innerW = $window.innerWidth;
            var innerH = $window.innerHeight;
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

    function updateState(rfb, state, oldstate, msg) {
        var s, sb, cad, level;
        s = angular.element(document.querySelector('#noVNC_status'))[0];
        sb = angular.element(document.querySelector('#noVNC_status_bar'))[0];
        switch (state) {
            case 'failed':       level = "error";  break;
            case 'fatal':        level = "error";  break;
            case 'normal':       level = "normal"; break;
            case 'disconnected': level = "normal"; break;
            case 'loaded':       level = "normal"; break;
            default:             level = "warn";   break;
        }

        if (typeof(msg) !== 'undefined') {
            // at this point, it's possible the window has already been destroyed, so make sure
            // the handles exist before writing.
            if (typeof(sb) !== 'undefined'){
                sb.setAttribute("class", "noVNC_status_" + level);
            }
            if (typeof(sb) !== 'undefined'){
                s.innerHTML = msg;
            }
        }
    }

    var resizeTimeout;
    $window.onresize = function () {
        // When the window has been resized, wait until the size remains
        // the same for 0.5 seconds before sending the request for changing
        // the resolution of the session
        clearTimeout(resizeTimeout);
        resizeTimeout = setTimeout(function(){
            UIresize();
        }, 500);
    }; 
}]);