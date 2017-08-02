angular.module('bmcApp')
    .controller(
        'sensorController',
        [
          '$scope', '$http', '$location', 'websocketService',
          function($scope, $http, $location, websocketService) {
            $scope.smartTablePageSize = 10;
            $scope.next_id = 0;
            websocketService.start('/dbus_monitor', function(evt) {
              var obj = JSON.parse(evt.data);

              $scope.$apply(function() {
                for (var sensor_name in obj) {
                  var found = false;
                  for (var sensor_index in $scope.rowCollection) {
                    var sensor_object = $scope.rowCollection[sensor_index];
                    if (sensor_object.name === sensor_name) {
                      sensor_object.value = obj[sensor_name];
                      found = true;
                      break;
                    }
                    }
                  if (!found) {
                    console.log(sensor_name + ' -> ' + obj[sensor_name]);
                    $scope.next_id = $scope.next_id + 1;

                    $scope.rowCollection.push({
                      id : $scope.next_id,
                      name : sensor_name,
                      value : obj[sensor_name],
                    });
                  }
                };
              });
            });

            $scope.rowCollection = [];

          }
        ])
    .factory('websocketService', [
      '$location',
      function($location) {
        return {
          start: function(url, callback) {
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