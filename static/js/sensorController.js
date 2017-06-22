angular.module('bmcApp').controller('sensorController', [
  '$scope', '$http', '$location', 'websocketService',
  function($scope, $http, $location, websocketService) {
    $scope.sensor_values = {};

    var host = $location.host();
    var port = $location.port();
    var protocol = "ws://";
    if ($location.protocol() === 'https') {
      protocol = 'wss://';
    }
    websocketService.start(protocol + host + ":" + port + "/sensorws", function (evt) {
        var obj = JSON.parse(evt.data);
        $scope.$apply(function () {
            for (var key in obj) {
              if (obj.hasOwnProperty(key)) {
                console.log(key + " -> " + obj[key]);
                $scope.sensor_values[key] = obj[key];
              }
            }
        });
    });

  }
]);

app.factory('websocketService', function () {
        return {
            start: function (url, callback) {
                var websocket = new WebSocket(url);
                websocket.onopen = function () {
                };
                websocket.onclose = function () {
                };
                websocket.onmessage = function (evt) {
                    callback(evt);
                };
            }
        }
    }
);