angular.module('bmcApp').factory('IPMIData', [
  '$websocket', '$location',
  function($websocket, $location) {

    var host = $location.host();
    var port = $location.port();
    var protocol = "ws://";
    if ($location.protocol() === 'https') {
      protocol = 'wss://';
    }
    // Open a WebSocket connection
    var dataStream = $websocket(protocol + host + port + "/ipmiws");
    var blob = new Blob([0x06, 0x00, 0xff, 0x07, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x20, 0x18, 0xc8, 0x81, 0x04, 0x38, 0x8e, 0x04, 0xb1]);
    dataStream.send(blob); 
    
    dataStream.onMessage(function(message) {
      collection.push(JSON.parse(message.data));
    });

    var methods = {

      get : function() { 
          var blob = new Blob([0x06, 0x00, 0xff, 0x07, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x20, 0x18, 0xc8, 0x81, 0x04, 0x38, 0x8e, 0x04, 0xb1]);
          dataStream.send(blob); 
      }
    };

    return methods;
  }
]);
angular.module('bmcApp').controller('ipmiController', [
  '$scope', 'IPMIData', function($scope, IPMIData) { $scope.IPMIData = IPMIData; }
]);