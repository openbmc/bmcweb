angular.module('bmcApp').controller('fwupdateController', [
  '$scope', '$http', '$uibModal', '$state',
  function($scope, $http, $uibModal, $state) {
    $scope.files = [];
    $scope.$watch('files', function(newValue, oldValue) {
      if (newValue.length > 0) {
        console.log('Loading firware file ' + $scope.files[0]);
        r = new FileReader();
        r.onload = function(e) {
          get_image_info = function(buffer) {
            image_info = {'valid' : false};
            var expected = '*SignedImage*\0\0\0';

            var dv1 = new Int8Array(e.target.result, 0, 16);

            for (var i = 0; i != expected.length; i++) {
              if (dv1[i] != expected.charCodeAt(i)) {
                return image_info;
              }
            }
            image_info['valid'] = true;
            var generation = new Int8Array(e.target.result, 16, 17)[0];
            image_info['generation'] = generation;
            if ((generation < 4) ||
                (generation > 5)) {  // not VLN generation header

              return image_info;
            } else {
              var version_minor = new Uint16Array(e.target.result, 20, 22)[0];
              image_info['major_version'] =
                  new Uint8Array(e.target.result, 28, 29)[0];
              image_info['submajor_version'] =
                  new Uint8Array(e.target.result, 29, 30)[0].toString(16);
              var version_minor2 = new Uint16Array(e.target.result, 30, 32)[0];
              image_info['sha1_version'] =
                  ('0000' + version_minor2.toString(16)).substr(-4) +
                  ('0000' + version_minor.toString(16)).substr(-4);
            }
            return image_info;
          };
          var image_info = get_image_info(e.target.result);
          $scope.image_info = image_info;

          var objectSelectionModal = $uibModal.open({
            templateUrl : 'static/partial-fwupdateconfirm.html',
            controller : function($scope) {
              $scope.image_info = image_info;
              $scope.file_to_load = file_to_load;
              // The function that is called for modal closing (positive button)

              $scope.okModal = function() {
                // Closing the model with result
                objectSelectionModal.close($scope.selection);
                $http({
                  method : 'POST',
                  url : '/intel/firmwareupload',
                  data : e.target.result,
                  transformRequest : [],
                  headers : {'Content-Type' : 'application/octet-stream'}
                })
                    .then(
                        function successCallback(response) {
                          console.log('Success uploaded. Response: ' +
                                      response.data)
                        },
                        function errorCallback(response) {
                          console.log('Error status: ' + response.status)
                        });
              };

              // The function that is called for modal dismissal(negative
              // button)

              $scope.dismissModal = function() {
                objectSelectionModal.dismiss();
              };
            }
          });
        };
        var file_to_load = $scope.files[0];
        $scope.file_to_load = $scope.files[0];
        r.readAsArrayBuffer($scope.files[0]);
      }
    });

  }
]);