/**
 * AngularJS module that adds support for ui-bootstrap modal states when using ui-router.
 *
 * @link https://github.com/nonplus/angular-ui-router-uib-modal
 *
 * @license angular-ui-router-uib-modal v0.0.11
 * (c) Copyright Stepan Riha <github@nonplus.net>
 * License MIT
 */

(function(angular) {

"use strict";
angular.module("ui.router.modal", ["ui.router"])
    .config(["$stateProvider", function ($stateProvider) {
        var stateProviderState = $stateProvider.state;
        $stateProvider["state"] = state;
        function state(name, config) {
            var stateName;
            var options;
            // check for $stateProvider.state({name: "state", ...}) usage
            if (angular.isObject(name)) {
                options = name;
                stateName = options.name;
            }
            else {
                options = config;
                stateName = name;
            }
            if (options.modal) {
                if (options.onEnter) {
                    throw new Error("Invalid modal state definition: The onEnter setting may not be specified.");
                }
                if (options.onExit) {
                    throw new Error("Invalid modal state definition: The onExit setting may not be specified.");
                }
                var openModal_1;
                // Get modal.resolve keys from state.modal or state.resolve
                var resolve_1 = (Array.isArray(options.modal) ? options.modal : []).concat(Object.keys(options.resolve || {}));
                var inject_1 = ["$uibModal", "$state"];
                options.onEnter = function ($uibModal, $state) {
                    // Add resolved values to modal options
                    if (resolve_1.length) {
                        options.resolve = {};
                        for (var i = 0; i < resolve_1.length; i++) {
                            options.resolve[resolve_1[i]] = injectedConstant(arguments[inject_1.length + i]);
                        }
                    }
                    var thisModal = openModal_1 = $uibModal.open(options);
                    openModal_1.result['finally'](function () {
                        if (thisModal === openModal_1) {
                            // Dialog was closed via $uibModalInstance.close/dismiss, go to our parent state
                            $state.go($state.get("^", stateName).name);
                        }
                    });
                };
                // Make sure that onEnter receives state.resolve configuration
                options.onEnter["$inject"] = inject_1.concat(resolve_1);
                options.onExit = function () {
                    if (openModal_1) {
                        // State has changed while dialog was open
                        openModal_1.close();
                        openModal_1 = null;
                    }
                };
            }
            return stateProviderState.call($stateProvider, stateName, options);
        }
    }]);
function injectedConstant(val) {
    return [function () { return val; }];
}


})(window.angular);