angular.module('bmcApp').controller('sensorController', [
  '$scope', '$http',
  function($scope, $http) {
    $http.get('/sensortest').then(function(sensor_values) {
      $scope.sensor_values = sensor_values;
    })
  }
]);