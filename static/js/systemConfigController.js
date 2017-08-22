angular.module('bmcApp')
    .controller(
        'systemConfigController', [
            '$scope', '$http',
            function($scope, $http) {
                $http.get('/intel/system_config').then(function(response) {
                    $scope.configuration = response.data;
                });
            }
        ]
    );
