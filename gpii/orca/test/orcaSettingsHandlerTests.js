/*
Orca Settings Handler Tests

Copyright 2013 Emergya

Licensed under the New BSD license. You may not use this file except in
compliance with this License.

You may obtain a copy of the License at
https://github.com/gpii/universal/LICENSE.txt
*/

"use strict";

var fluid = require("universal"),
    jqUnit = fluid.require("jqUnit");

require("orca");
var orca = fluid.registerNamespace("gpii.orca");

jqUnit.module("OrcaSettingsHandler Module");

jqUnit.test("Running tests for Orca Settings Handler", function () {
    jqUnit.expect(10);

    var payload = {
        "org.gnome.orca": [{
            options: {
                "user": "test1"
            },
            settings: {
                "enableBraille": true,
                "enableEchoByWord": true,
                "enableEchoByCharacter": false,
                "voices.default.rate": 100,
                "enableTutorialMessages": false,
                "voices.default.family": { "locale": "es", "name": "spanish-latin-american" },
                "verbalizePunctuationStyle": 0
            }
        }]
    };

    var returnPayload = orca.set(payload);

    // Check if profile exists

    jqUnit.assertTrue("Profile 'test1' exists", returnPayload["org.gnome.orca"][0].settings.profiles.newValue.test1);

    // Check if 'test1' is the default starting profile

    var actual = returnPayload["org.gnome.orca"][0].settings["general.startingProfile"].newValue;
    jqUnit.assertDeepEq("'test1' is the new starting profile", ["test1", "test1"], actual);

    // Check for specific one-to-one settings from the payload

    for (var setting in ["enableBraille", "enableEchoByWord", "enableEchoByCharacter", "enableTutorialMessages", "verbalizePunctuationStyle"]) {
        jqUnit.assertDeepEq("Checking " + setting + " setting",
                            payload["org.gnome.orca"][0].settings[setting],
                            returnPayload["org.gnome.orca"][0].settings.profiles.newValue.test1[setting]);
    }

    // Check for voices' stuff
    //
    jqUnit.assertDeepEq("Checking for voices.default.rate setting",
                        payload["org.gnome.orca"][0].settings["voices.default.rate"],
                        returnPayload["org.gnome.orca"][0].settings.profiles.newValue.test1.voices["default"].rate);

    jqUnit.assertDeepEq("Checking for voices.default.family setting",
                        payload["org.gnome.orca"][0].settings["voices.default.family"],
                        returnPayload["org.gnome.orca"][0].settings.profiles.newValue.test1.voices["default"].family);

    // Let's simulate a logout and restore the settings file into its initial state
    //
    var newPayload = fluid.copy(payload);
    newPayload["org.gnome.orca"][0].settings.profiles = returnPayload["org.gnome.orca"][0].settings.profiles.oldValue;
    newPayload["org.gnome.orca"][0].settings["general.startingProfile"] = returnPayload["org.gnome.orca"][0].settings["general.startingProfile"].oldValue;
    var newReturnPayload = orca.set(newPayload);

    // Check if 'test1' profile has been removed successfully
    //
    jqUnit.assertFalse("Profile 'test1' does not exists", newReturnPayload["org.gnome.orca"][0].settings.profiles.newValue.test1);

});