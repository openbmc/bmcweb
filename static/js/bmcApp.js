'use strict';
angular.module('Authentication', []);
var app = angular.module('bmcApp', [
  'Authentication', 'ngCookies', 'ui.bootstrap', 'ui.router', 'restangular',
  'ngSanitize', 'ngWebSocket'
]);


app.controller('MainCtrl', function($scope, Restangular) {

});

app.run(['$rootScope', '$cookieStore', '$state', 'Restangular', 'AuthenticationService',
  function($rootScope, $cookieStore, $state, Restangular, AuthenticationService) {
    if ($rootScope.globals == undefined){
        $rootScope.globals = {};
    }
    
    // keep user logged in after page refresh
    AuthenticationService.RestoreCredientials();

    $rootScope.$on(
        '$stateChangeStart',
        function(event, toState, toParams, fromState, fromParams, options) {
          // redirect to login page if not logged in
          if (toState.name !== 'login' && !$rootScope.globals.currentUser) {
            // If logged out and transitioning to a logged in page:
            event.preventDefault();
            $state.go('login');
          }
        });

    // RestangularProvider.setDefaultHttpFields({cache: true});
    Restangular.setErrorInterceptor(function(response) {
      if (response.status == 401) {
        console.log("Login required... ");

        var invalidate_reason = "Your user was logged out.";
        var continue_promise_chain = false;

        // if we're attempting to log in, we need to
        // continue the promise chain to make sure the user is informed
        if ($state.current.name === "login") {
          invalidate_reason = "Your username and password was incorrect";
          continue_promise_chain = true
        } else {
          $state.after_login_state = $state.current.name;
          $state.go('login');
        }
        AuthenticationService.ClearCredentials(invalidate_reason);

        return continue_promise_chain;  // stop the promise chain
      } else if (response.status == 404) {
        console.log("Resource not available...");
      } else {
        console.log(
            "Response received with HTTP error code: " + response.status);
      }

    });

  }
]);

app.config(function(RestangularProvider) {
  // set the base url for api calls on our RESTful services
  var newBaseUrl = "";

  var deployedAt = window.location.href.substring(0, window.location.href);
  newBaseUrl = deployedAt + "/restui";

  RestangularProvider.setBaseUrl(newBaseUrl);
});

app.config(function($stateProvider, $urlRouterProvider) {

  $urlRouterProvider.otherwise('/systeminfo');

  $stateProvider
      // nested list with just some random string data
      .state('login', {
        url: '/login',
        templateUrl: 'login.html',
        controller: 'LoginController',
      })
      // systeminfo view ========================================
      .state(
          'systeminfo',
          {url: '/systeminfo', templateUrl: 'partial-systeminfo.html'})


      // HOME STATES AND NESTED VIEWS ========================================
      .state(
          'eventlog', {url: '/eventlog', templateUrl: 'partial-eventlog.html'})


      .state(
          'kvm', {url: '/kvm', templateUrl: 'partial-kvm.html'})

      // ABOUT PAGE AND MULTIPLE NAMED VIEWS =================================
      .state('about', {url: '/about', templateUrl: 'partial-fruinfo.html'})

      // nested list with custom controller
      .state('about.list', {
        url: '/list',
        templateUrl: 'partial-home-list.html',
        controller: function($scope) {
          $scope.dogs = ['Bernese', 'Husky', 'Goldendoodle'];
        }
      })


});

app.controller('PaginationDemoCtrl', function($scope, $log) {
  $scope.totalItems = 64;
  $scope.currentPage = 4;

  $scope.setPage = function(pageNo) { $scope.currentPage = pageNo; };

  $scope.pageChanged = function() {
    $log.log('Page changed to: ' + $scope.currentPage);
  };

  $scope.maxSize = 5;
  $scope.bigTotalItems = 175;
  $scope.bigCurrentPage = 1;
});

angular
    .module('Authentication')

    .factory(
        'AuthenticationService',
        [
          'Restangular', '$cookieStore', '$rootScope', '$timeout',
          function(Restangular, $cookieStore, $rootScope, $timeout) {
            var service = {};

            service.Login = function(
                username, password, success_callback, fail_callback) {

              var user = {"username": username, "password": password};
              Restangular.all("login").post(user).then(
                  success_callback, fail_callback);
            };

            service.SetCredentials = function(username, token) {
              $rootScope.globals["currentUser"] = {username: username, authdata: token};
              Restangular.setDefaultHeaders(
                  {'Authorization': 'Token ' + token});
              $cookieStore.put('globals', $rootScope.globals);
            };

            service.ClearCredentials = function(reason) {
              $rootScope.globals["currentUser"] = null;
              if (reason !== null) {
                service.logoutreason = reason;
              }
              $cookieStore.remove('globals');
              Restangular.setDefaultHeaders({});
            };

            service.RestoreCredientials = function() {
              var globals = $cookieStore.get('globals') || {};
              if (globals.currentUser) {
                service.SetCredentials(
                    globals.currentUser.username, globals.currentUser.authdata);
              }
            };

            service.logoutreason = "";
            return service;
          }
        ])

    .factory('Websocket_URI', 
        function($rootScope, $http) {
          var loc = window.location, websocket_uri;
          if (loc.protocol === "https:") {
            websocket_uri = "wss:";
          } else {
            websocket_uri = "ws:";
          }
          websocket_uri += "//" + loc.hostname + ":9000";
          // Append the authentication token
          websocket_uri += "?token="
          websocket_uri += $rootScope.globals["currentUser"]["authdata"]
          var methods = {
            uri: websocket_uri
          }
          return methods;
        })
    .factory('Base64', function() {
      /* jshint ignore:start */

      var keyStr =
          'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=';

      return {
        encode: function(input) {
          var output = "";
          var chr1, chr2, chr3 = "";
          var enc1, enc2, enc3, enc4 = "";
          var i = 0;

          do {
            chr1 = input.charCodeAt(i++);
            chr2 = input.charCodeAt(i++);
            chr3 = input.charCodeAt(i++);

            enc1 = chr1 >> 2;
            enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
            enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
            enc4 = chr3 & 63;

            if (isNaN(chr2)) {
              enc3 = enc4 = 64;
            } else if (isNaN(chr3)) {
              enc4 = 64;
            }

            output = output + keyStr.charAt(enc1) + keyStr.charAt(enc2) +
                keyStr.charAt(enc3) + keyStr.charAt(enc4);
            chr1 = chr2 = chr3 = "";
            enc1 = enc2 = enc3 = enc4 = "";
          } while (i < input.length);

          return output;
        },

        decode: function(input) {
          var output = "";
          var chr1, chr2, chr3 = "";
          var enc1, enc2, enc3, enc4 = "";
          var i = 0;

          // remove all characters that are not A-Z, a-z, 0-9, +, /, or =
          var base64test = /[^A-Za-z0-9\+\/\=]/g;
          if (base64test.exec(input)) {
            window.alert(
                "There were invalid base64 characters in the input text.\n" +
                "Valid base64 characters are A-Z, a-z, 0-9, '+', '/',and '='\n" +
                "Expect errors in decoding.");
          }
          input = input.replace(/[^A-Za-z0-9\+\/\=]/g, "");

          do {
            enc1 = keyStr.indexOf(input.charAt(i++));
            enc2 = keyStr.indexOf(input.charAt(i++));
            enc3 = keyStr.indexOf(input.charAt(i++));
            enc4 = keyStr.indexOf(input.charAt(i++));

            chr1 = (enc1 << 2) | (enc2 >> 4);
            chr2 = ((enc2 & 15) << 4) | (enc3 >> 2);
            chr3 = ((enc3 & 3) << 6) | enc4;

            output = output + String.fromCharCode(chr1);

            if (enc3 != 64) {
              output = output + String.fromCharCode(chr2);
            }
            if (enc4 != 64) {
              output = output + String.fromCharCode(chr3);
            }

            chr1 = chr2 = chr3 = "";
            enc1 = enc2 = enc3 = enc4 = "";

          } while (i < input.length);

          return output;
        }
      };

      /* jshint ignore:end */
    });
