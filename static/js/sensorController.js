angular.module('bmcApp').controller('sensorController', [
  '$scope', '$http', '$location', 'dbusWebsocketService',
  function($scope, $http, $location, dbusWebsocketService) {
    $scope.smartTablePageSize = 10;
    $scope.next_id = 0;
    dbusWebsocketService.start('/xyz/openbmc_project/sensors', function(evt) {
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
]);
