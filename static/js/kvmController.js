"use strict";
angular.module('bmcApp').controller('KvmController', 
['$scope', '$location', '$window',
function($scope, $location, $window) {

    /*jslint white: false */
    /*global window, $, Util, RFB, */

    var desktopName;

    WebUtil.init_logging(WebUtil.getConfigVar('logging', 'debug'));
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
                        'onDesktopName': updateDesktopName,
                        'foobar': 1
                        });
        rfb.connect(host, port, password, path);
    } catch (exc) {
        updateState(null, 'fatal', null, 'Unable to create RFB client -- ' + exc);
        return; // don't continue trying to connect
    };

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
    };
    function updateDesktopName(rfb, name) {
        desktopName = name;
    };
    function FBUComplete(rfb, fbu) {
        UIresize();
        rfb.set_onFBUComplete(function() { });
    };
    function sendCtrlAltDel() {
        rfb.sendCtrlAltDel();
        return false;
    };

    function status(text, level) {
        var status_bar = angular.element(document.querySelector('#noVNC_status_bar'))[0];
        // Need to check if the status bar still exists.  On page change, it gets destroyed
        // when we swap to a different view.  The system will disconnect async
        if (status_bar){
            status_bar.textContent = text;
        }

        var status = angular.element(document.querySelector('#noVNC_status'))[0];
        switch (level) {
            case 'normal':
            case 'warn':
            case 'error':
                break;
            default:
                level = "warn";
        }
        if (status){
            status.setAttribute("class", "noVNC_status_" + level);
        }
    };

    function updateState(rfb, state, oldstate) {
        switch (state) {
            case 'connecting':
                status("Connecting", "normal");
                break;
            case 'connected':
                if (rfb && rfb.get_encrypt()) {
                    status("Connected (encrypted) to " +
                            desktopName, "normal");
                } else {
                    status("Connected (unencrypted) to " +
                            desktopName, "normal");
                }
                break;
            case 'disconnecting':
                status("Disconnecting", "normal");
                break;
            case 'disconnected':
                status("Disconnected", "normal");
                break;
            default:
                status(state, "warn");
                break;
        }

    };

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