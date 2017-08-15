'use strict';
angular.module('Authentication', []);
var app = angular.module('bmcApp', [
  'ngCookies',
  'ngAnimate',
  'ngSanitize',
  'ui.bootstrap',
  'ui.router',
  'ngWebSocket',
  'Authentication',
  'ui.router.modal',
  'smart-table',
]);

app.service('loginInterceptor', [
  '$injector',
  function($injector) {
    var service = this;

    service.responseError = function(response) {
      var $state = $injector.get('$state');
      var AuthenticationService = $injector.get('AuthenticationService');
      if (response.status == 401) {
        console.log('Login required... ');

        var invalidate_reason = 'Your user was logged out.';

        // if we're attempting to log in, we need to
        // continue the promise chain to make sure the user is informed
        if ($state.current.name === 'login') {
          invalidate_reason = 'Your username and password was incorrect';
        } else {
          $state.after_login_state = $state.current.name;
          $state.go('login');
        }
        AuthenticationService.ClearCredentials(invalidate_reason);
        }
      return response;
    };
  }
])

app.config([
  '$httpProvider',
  function($httpProvider) {
    $httpProvider.interceptors.push('loginInterceptor');
  }
]);

app.directive('windowSize', [
  '$window',
  function($window) {
    return function(scope, element) {
      var w = angular.element($window);
      scope.getWindowDimensions = function() {
        return {'h' : w.height(), 'w' : w.width()};
      };
      scope.$watch(scope.getWindowDimensions, function(newValue, oldValue) {
        scope.windowHeight = newValue.h;
        scope.windowWidth = newValue.w;
        scope.style = function() {
          return {
            'height' : (newValue.h - 100) + 'px',
            'width' : (newValue.w - 100) + 'px'
          };
        };
      }, true);

      w.bind('resize', function() { scope.$apply(); });
    }
  }
]);

app.directive('fileInput', ['$parse', function ($parse) {
    return {
        restrict: 'A',
        link: function (scope, element, attributes) {
            element.bind('change', function () {
                $parse(attributes.fileInput)
                .assign(scope,element[0].files)
                scope.$apply()
            });
        }
    };
}]);

app.run([
  '$rootScope', '$cookieStore', '$state', 'AuthenticationService', '$http',
  '$templateCache',
  function($rootScope, $cookieStore, $state, AuthenticationService, $http,
           $templateCache) {

    if ($rootScope.globals == undefined) {
      $rootScope.globals = {};
    }

    // keep user logged in after page refresh
    AuthenticationService.RestoreCredientials();

    $rootScope.$on(
        '$stateChangeStart',
        function(event, toState, toParams, fromState, fromParams, options) {
          // redirect to login page if not logged in
          // unless we're already trying to go to the login page (prevent a
          // loop)
          if (!$rootScope.globals.currentUser && toState.name !== 'login') {
            // If logged out and transitioning to a logged in page:
            event.preventDefault();
            $state.go('login');
          }
        });

  }
]);

app.config([
  '$stateProvider', '$urlRouterProvider',
  function($stateProvider, $urlRouterProvider) {

    $urlRouterProvider.otherwise('/systeminfo');

    $stateProvider
        .state('login', {
          url : '/login',
          templateUrl : 'static/partial-login.html',
          controller : 'LoginController',
        })
        .state('systeminfo', {
          url : '/systeminfo',
          templateUrl : 'static/partial-systeminfo.html'
        })
        .state(
            'eventlog',
            {url : '/eventlog', templateUrl : 'static/partial-eventlog.html'})

        .state('kvm', {url : '/kvm', templateUrl : 'static/partial-kvm.html'})

        .state('ipmi',
               {url : '/ipmi', templateUrl : 'static/partial-ipmi.html'})

        .state('sensor',
               {url : '/sensor', templateUrl : 'static/partial-sensor.html'})

        .state('systemconfig',
              {url: '/systemconfig', templateUrl : 'static/partial-systemconfig.html'})

        .state(
            'fwupdate',
            {url : '/fwupdate', templateUrl : 'static/partial-fwupdate.html'})
        // nested list with custom controller
        .state('fwupdate.confirm', {
          url : '/confirm',
          templateUrl : 'static/partial-fwupdateconfirm.html',
          modal : true
        })
        // ABOUT PAGE AND MULTIPLE NAMED VIEWS =================================
        .state('about',
               {url : '/about', templateUrl : 'static/partial-fruinfo.html'})

        // nested list with custom controller
        .state('about.list', {
          url : '/list',
          templateUrl : 'static/partial-home-list.html',
          controller : function($scope) {
            $scope.dogs = [ 'Bernese', 'Husky', 'Goldendoodle' ];
          }
        });

  }
]);

app.directive('fileread', [ function() {
                return {
                  scope: {fileread : '='},
                      link: function(scope, element, attributes) {
                        element.bind('change', function(changeEvent) {
                          scope.$apply(function() {
                            scope.fileread = changeEvent.target.files[0];
                            // or all selected files:
                            // scope.fileread = changeEvent.target.files;
                          });
                        });
                      }
                }
              } ]);

app.controller('PaginationDemoCtrl', [
  '$scope', '$log',
  function($scope, $log) {
    $scope.totalItems = 64;
    $scope.currentPage = 4;

    $scope.setPage = function(pageNo) { $scope.currentPage = pageNo; };

    $scope.pageChanged = function() {
      $log.log('Page changed to: ' + $scope.currentPage);
    };

    $scope.maxSize = 5;
    $scope.bigTotalItems = 175;
    $scope.bigCurrentPage = 1;
  }
]);

angular.module('Authentication').factory('AuthenticationService', [
  '$cookieStore', '$rootScope', '$timeout', '$log', '$http',
  function($cookieStore, $rootScope, $timeout, $log, $http) {
    var service = {};

    service.Login = function(username, password) {
      var user = {'username' : username, 'password' : password};
      return $http.post('/login', user);
    };

    service.SetCredentials = function(username, token) {
      $rootScope.globals['currentUser'] = {
        username : username,
        authdata : token
      };
      $http.defaults.headers.common['Authorization'] = 'Token ' + token;
      $cookieStore.put('globals', $rootScope.globals);
    };

    service.ClearCredentials = function(reason) {
      $rootScope.globals['currentUser'] = null;
      if (reason !== null) {
        service.logoutreason = reason;
      }
      $cookieStore.remove('globals');
      $http.defaults.headers.common['Authorization'] = '';
    };

    service.RestoreCredientials = function() {
      var globals = $cookieStore.get('globals') || {};
      if (globals.currentUser) {
        service.SetCredentials(globals.currentUser.username,
                               globals.currentUser.authdata);
      }
    };

    service.IsLoggedIn = function() {
      if ($rootScope.globals['currentUser']) {
        return true;
      } else {
        return false;
      }
    };

    service.logoutreason = '';
    return service;
  }
]);
