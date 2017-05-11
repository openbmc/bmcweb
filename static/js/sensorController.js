angular.module('bmcApp').controller('sensorController', [
  '$scope', '$resource',
  function($scope, $resource) {

    var systeminfo = $resource("/sensortest");
    systeminfo.get(function(sensor_values) {
      $scope.sensor_values = sensor_values;
    });

  }
]);