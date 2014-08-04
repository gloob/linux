/*!
GPII Node.js GSettings Bridge

Copyright 2012 Steven Githens

Licensed under the New BSD license. You may not use this file except in
compliance with this License.

You may obtain a copy of the License at
https://github.com/gpii/universal/LICENSE.txt
*/

(function () {
    "use strict";

    var fluid = require("universal");
    var gpii = fluid.registerNamespace("gpii");
    var nodeGSettings = require("./nodegsettings/build/Release/nodegsettings.node");

    fluid.registerNamespace("gpii.launch");
    fluid.registerNamespace("gpii.gsettings");

    fluid.defaults("gpii.gsettings.setSingleKey", {
        gradeNames: "fluid.function",
        argumentMap: {
            schemaId: 0,
            key: 1,
            value: 2
        }
    });

    fluid.defaults("gpii.gsettings.getSingleKey", {
        gradeNames: "fluid.function",
        argumentMap: {
            schemaId: 0,
            key: 1
        }
    });

    gpii.gsettings.getSingleKey = function(schemaId, key) {
        return nodeGSettings.get_gsetting(schemaId,key);
    };

    gpii.gsettings.setSingleKey = function(schemaId, key, value) {
        nodeGSettings.set_gsetting(schemaId, key, value);
    };

    gpii.gsettings.get = function(settingsarray) {
        var app = fluid.copy(settingsarray);
        for (var appId in app) {
            for (var j = 0; j < app[appId].length; j++) {
                var schemaId = app[appId][j].options.schema;
                var settings = app[appId][j].settings;
                var keys = nodeGSettings.get_gsetting_keys(schemaId);

                if (settings === null) {
                    settings = {};
                    for (var k = 0; k < keys.length; k++) {
                        var key = keys[k];
                        settings[key] = nodeGSettings.get_gsetting(schemaId, key);
                    }
                }
                else {
                    for (var settingKey in settings) {
                        if (keys.indexOf(settingKey) === -1) {
                            continue;
                        }
                        settings[settingKey] = nodeGSettings.get_gsetting(schemaId,settingKey);
                    }
                }
                var noOptions = { settings: settings };
                app[appId][j] = noOptions;
            }
        }
        return app;
    };

    gpii.gsettings.set = function(settingsarray) {
        var app = fluid.copy(settingsarray);
        for (var appId in app) {
            for (var j = 0; j < app[appId].length; j++) {
                var schemaId = app[appId][j].options.schema;
                var settings = app[appId][j].settings;
                var keys = nodeGSettings.get_gsetting_keys(schemaId);

                for (var settingKey in settings) {
                    if (keys.indexOf(settingKey) === -1) {
                        continue;
                    }
                    var value = settings[settingKey];
                    var oldValue = nodeGSettings.get_gsetting(schemaId,settingKey);
                    nodeGSettings.set_gsetting(schemaId,settingKey,value);
                    settings[settingKey] = {
                        "oldValue": oldValue,
                        "newValue": value
                    };
                }
                var noOptions = { settings: settings};
                app[appId][j] = noOptions;
            }
        }
        return app;
    };

})();