angular.module('bmcApp').controller('MainCtrl', [
  '$scope', 'AuthenticationService',
  function($scope, AuthenticationService) {
    $scope.is_logged_in = AuthenticationService.IsLoggedIn;
  }
]);