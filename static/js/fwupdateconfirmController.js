angular.module('bmcApp').controller('fwupdateconfirmController', [
  '$scope', '$stateParams',{
    $scope.filename = $stateParams.filename;
  }
]);