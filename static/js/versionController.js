angular.module('bmcApp').controller('versionController', function($scope, $resource) {


    var systeminfo = $resource("/systeminfo");
    systeminfo.get(function(systeminfo){
        $scope.host_power_on= true;
        $scope.rmm_module_installed= true;
        $scope.bmc_available= true;
        $scope.bmc_build_date= new Date(2016, 0, 1, 2, 3, 4, 567);
        $scope.bios_build_number = "D0191";
        $scope.bmc_build_number = "96.37";
        $scope.bmc_build_extended = "e04989f7";
        $scope.bmc_backup_build_number = "96.37";
        $scope.bmc_backup_build_extended = "e04989f7";
    });



});