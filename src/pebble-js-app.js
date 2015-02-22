var INSTALLED_SETTINGS_VERSION = 1;
var CONSOLE_LOG = false;

Pebble.addEventListener("ready",
  function(e) {
    consoleLog("Event listener - ready");
  }
);

Pebble.addEventListener("showConfiguration",
  function(e) {
    consoleLog("Event listener - showConfiguration");
    var settingsUrl = "http://www.sherbeck.com/pebble/wiper.html?" + formatUrlVariables();
    consoleLog("Opening settings at " + settingsUrl);
    Pebble.openURL(settingsUrl);
  }
);

Pebble.addEventListener("appmessage",
  function(e) {
    consoleLog("Event listener - appmessage");

    if (typeof(e.payload.KEY_USAGE_DURATION) !== "undefined") {
      var message = "Usage duration is " + e.payload.KEY_USAGE_DURATION;
      consoleLog(message);
      recordUsageDuration(e.payload.KEY_USAGE_DURATION);
    }
  }
);

Pebble.addEventListener("webviewclosed",
  function(e) {
    consoleLog("Event listener - webviewclosed");
    
    if (typeof(e.response) === "undefined" || e.response == "CANCELLED") {
      consoleLog("Settings canceled by user");
      
    } else {
      var configuration = JSON.parse(decodeURIComponent(e.response));
      consoleLog("Configuration window returned: " + JSON.stringify(configuration));
      
      saveSettings(configuration);

      var dictionary = {
        "KEY_BLUETOOTH_VIBRATE" : parseInt(configuration.bluetoothVibrate)
      };
  
      Pebble.sendAppMessage(dictionary,
        function(e) {
          consoleLog("Settings successfully sent to Pebble");
        },
        function(e) {
          consoleLog("Error sending settings to Pebble");
        }
      );
    }
  }
);

function formatUrlVariables() {
  var bluetoothVibrate = getLocalInt("bluetoothVibrate", 1);
  
  return ("installedSettingsVersion=" + INSTALLED_SETTINGS_VERSION + "&bluetoothVibrate=" + bluetoothVibrate +
          "&accountToken=" + Pebble.getAccountToken() + "&watchToken=" + Pebble.getWatchToken());
}

function saveSettings(settings) {
  localStorage.setItem("bluetoothVibrate", parseInt(settings.bluetoothVibrate)); 
}

function recordUsageDuration(duration) {
  var req = new XMLHttpRequest();
  var analyticsUrl = "http://www.sherbeck.com/pebble/analytics.txt?app=wiper" + "&usageDuration=" + duration +
                     "&accountToken=" + Pebble.getAccountToken() + "&watchToken=" + Pebble.getWatchToken();
  
  req.open("POST", analyticsUrl, true);
  req.send(null);  
}

function getLocalInt(name, defaultValue) {
  var localValue = parseInt(localStorage.getItem(name));
	if (isNaN(localValue)) {
		localValue = defaultValue;
	}
  
  return localValue;
}

function consoleLog(message) {
  if (CONSOLE_LOG) {
    console.log(message);
  }
}