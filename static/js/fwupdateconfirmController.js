angular.module('bmcApp').controller('fwupdateconfirmController', [
  '$scope', '$stateParams',function($scope, $stateParams) {
    $scope.filename = $stateParams.filename;
  }
]);