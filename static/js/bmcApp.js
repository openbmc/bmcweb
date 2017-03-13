'use strict';
angular.module('Authentication', []);
var app = angular.module('bmcApp', [
  'Authentication', 
  'ngCookies', 
  'ui.bootstrap', 
  'ui.router',
  'ngSanitize', 
  'ngWebSocket', 
  'ngResource'
]);


app.controller('MainCtrl', function($scope) {

});

app.service('loginInterceptor', ["$injector",
  function($injector) {
    var service = this;

    service.responseError = function(response) {
        var $state = $injector.get('$state');
        var AuthenticationService = $injector.get('AuthenticationService');
        if (response.status == 401){
          console.log("Login required... ");

          var invalidate_reason = "Your user was logged out.";

          // if we're attempting to log in, we need to
          // continue the promise chain to make sure the user is informed
          if ($state.current.name === "login") {
            invalidate_reason = "Your username and password was incorrect";
          } else {
            $state.after_login_state = $state.current.name;
            $state.go('login');
          }
          AuthenticationService.ClearCredentials(invalidate_reason);
        }

    };
}])

app.config(['$httpProvider', function ($httpProvider) {
    $httpProvider.interceptors.push('loginInterceptor');
}]);

app.directive('windowSize', function ($window) {
  return function (scope, element) {
    var w = angular.element($window);
    scope.getWindowDimensions = function () {
        return {
            'h': w.height(),
            'w': w.width()
        };
    };
    scope.$watch(scope.getWindowDimensions, function (newValue, oldValue) {
      scope.windowHeight = newValue.h;
      scope.windowWidth = newValue.w;
      scope.style = function () {
          return {
              'height': (newValue.h - 100) + 'px',
              'width': (newValue.w - 100) + 'px'
          };
      };
    }, true);

    w.bind('resize', function () {
        scope.$apply();
    });
  }
});

app.run(['$rootScope', '$cookieStore', '$state', '$resource', 'AuthenticationService',
  function($rootScope, $cookieStore, $state, $resource, AuthenticationService) {
    if ($rootScope.globals == undefined){
        $rootScope.globals = {};
    }
    
    // keep user logged in after page refresh
    AuthenticationService.RestoreCredientials();

    $rootScope.$on(
        '$stateChangeStart',
        function(event, toState, toParams, fromState, fromParams, options) {
          // redirect to login page if not logged in
          // unless we're already trying to go to the login page (prevent a loop)
          if (!$rootScope.globals.currentUser && toState.name !== 'login') {
            // If logged out and transitioning to a logged in page:
            event.preventDefault();
            $state.go('login');
          }
        });
  }
]);

app.config(['$stateProvider', '$urlRouterProvider', 
    function($stateProvider, $urlRouterProvider) {

  $urlRouterProvider.otherwise('/systeminfo');

  $stateProvider
      // nested list with just some random string data
      .state('login', {
        url: '/login',
        templateUrl: 'static/login.html',
        controller: 'LoginController',
      })
      // systeminfo view ========================================
      .state(
          'systeminfo',
          {url: '/systeminfo', templateUrl: 'static/partial-systeminfo.html'})


      // HOME STATES AND NESTED VIEWS ========================================
      .state(
          'eventlog', {url: '/eventlog', templateUrl: 'static/partial-eventlog.html'})


      .state(
          'kvm', {url: '/kvm', templateUrl: 'static/partial-kvm.html'})

      // ABOUT PAGE AND MULTIPLE NAMED VIEWS =================================
      .state('about', {url: '/about', templateUrl: 'static/partial-fruinfo.html'})

      // nested list with custom controller
      .state('about.list', {
        url: '/list',
        templateUrl: 'static/partial-home-list.html',
        controller: function($scope) {
          $scope.dogs = ['Bernese', 'Husky', 'Goldendoodle'];
        }
      })


}]);

app.controller('PaginationDemoCtrl', ['$scope', '$log', function($scope, $log) {
  $scope.totalItems = 64;
  $scope.currentPage = 4;

  $scope.setPage = function(pageNo) { $scope.currentPage = pageNo; };

  $scope.pageChanged = function() {
    $log.log('Page changed to: ' + $scope.currentPage);
  };

  $scope.maxSize = 5;
  $scope.bigTotalItems = 175;
  $scope.bigCurrentPage = 1;
}]);

angular.module('Authentication').factory(
        'AuthenticationService',
        ['$cookieStore', '$rootScope', '$timeout', '$resource', '$log', '$http',
          function($cookieStore, $rootScope, $timeout, $resource, $log, $http) {
            var service = {};

            service.Login = function(username, password, success_callback, fail_callback) {

              var user = {"username": username, "password": password};
              var UserLogin = $resource("/login");
              var this_login = new UserLogin();
              this_login.data = {"username": username, "password": password};
              UserLogin.save(user, success_callback, fail_callback);
              
            };

            service.SetCredentials = function(username, token) {
              $rootScope.globals["currentUser"] = {username: username, authdata: token};
              $http.defaults.headers.common['Authorization'] = 'Token ' + token;
              $cookieStore.put('globals', $rootScope.globals);
            };

            service.ClearCredentials = function(reason) {
              $rootScope.globals["currentUser"] = null;
              if (reason !== null) {
                service.logoutreason = reason;
              }
              $cookieStore.remove('globals');
              $http.defaults.headers.common['Authorization'] = '';
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
