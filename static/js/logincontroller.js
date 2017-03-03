angular.module('Authentication').controller('LoginController', [
    '$scope', '$rootScope', '$location', '$state', 'AuthenticationService',
    function($scope, $rootScope, $location, $state, AuthenticationService) {
    $scope.logoutreason = AuthenticationService.logoutreason;

    $scope.login = function() {
        $scope.dataLoading = true;
        AuthenticationService.Login(
            $scope.username, $scope.password,
            function(response) {
            AuthenticationService.SetCredentials(
                $scope.username, response.token);
            if (typeof $state.after_login_state === "undefined") {
                $state.after_login_state = "systeminfo";
            }
            $state.go($state.after_login_state);
            delete $state.after_login_state;

            },
            function(response) {
            if (response.status === 401) {
                // reset login status
                AuthenticationService.ClearCredentials(
                    "Username or Password is incorrect");
            }
            $scope.logoutreason = AuthenticationService.logoutreason;
            $scope.error = response.message;
            $scope.dataLoading = false;
            });
    };
    }
]);