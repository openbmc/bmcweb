angular.module('bmcApp').factory('dbusWebsocketService', [
  '$location',
  function($location) {
    return {
      start: function(dbus_namespace, callback) {
        var url = '/dbus_monitor?path_namespace=' + dbus_namespace;
        var host = $location.host();
        var port = 18080;
        var protocol = 'wss://';
        if ($location.protocol() === 'http') {
          protocol = 'ws://';
        }
        var websocket = new WebSocket(protocol + host + ':' + port + url);
        websocket.onopen = function() {};
        websocket.onclose = function() {};
        websocket.onmessage = function(evt) { callback(evt); };
      }
    }
  }
]);