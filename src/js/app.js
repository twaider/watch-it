// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay
var clay = new Clay(clayConfig);
var messageKeys = require('message_keys');

var myAPIKey = '5ae4d297761d24a5e8f6be0af8fff507';
var ICONS = {
  '01d': 'a',
  '02d': 'b',
  '03d': 'c',
  '04d': 'd',
  '09d': 'e',
  '10d': 'f',
  '11d': 'g',
  '13d': 'h',
  '50d': 'i',
  '01n': 'a',
  '02n': 'b',
  '03n': 'c',
  '04n': 'd',
  '09n': 'e',
  '10n': 'f',
  '11n': 'g',
  '13n': 'h',
  '50n': 'i',
};

function parseIcon(icon) {
  return ICONS[icon];
}

function getLocalStorageItem(key) {
  var item = localStorage.getItem(key);
  if (item != 'null' && item != null && item != 'undefined' && item != 'None' &&
      item != '') {
    return item;
  }
  return false;
}

var xhrRequest = function(url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function() { callback(this.responseText); };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // Construct URL
  var units =
      getLocalStorageItem('UNITS') ? '&units=imperial' : '&units=metric';
  var loc = getLocalStorageItem('LOCATION') ?
      'q=' + getLocalStorageItem('LOCATION') :
      'lat=' + pos.coords.latitude + '&lon=' + pos.coords.longitude;
  var url = 'http://api.openweathermap.org/data/2.5/weather?' + loc +
      '&cnt=1&appid=' + myAPIKey + units;
  console.log('url: ' + url);

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', function(responseText) {
    // responseText contains a JSON object with weather info
    var json = JSON.parse(responseText);

    if (json.cod == '404') {
      console.log(json.message);
      return false;
    }

    // Temperature in Kelvin requires adjustment
    var temperature = json.main.temp;  // Math.round(json.main.temp - 273.15);
    console.log('Temperature is ' + temperature);

    // Conditions
    var icon = parseIcon(json.weather[0].icon);
    console.log('Icon ' + icon);
    console.log('Condition ' + json.weather[0].main);

    // Assemble dictionary using our keys
    var dictionary = {'TEMPERATURE': temperature, 'ICON': icon, 'WEATHER_STATUS': 1};

    // Send to Pebble
    Pebble.sendAppMessage(
        dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully!");
        },
        function(e) { console.log('Error sending weather info to Pebble!'); });
  });
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
      locationSuccess, locationError, {timeout: 15000, maximumAge: 60000});
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready!');
  // Get the initial weather if enabled
  if (getLocalStorageItem('WEATHER_ON')) {
    getWeather();
  }
});

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received!');
  getWeather();
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }

  // Get the keys and values from each config item
  var dict = clay.getSettings(e.response);
  var claySettings = clay.getSettings(e.response);

  if (claySettings[messageKeys.WEATHER_ON] == 1) {
    localStorage.setItem('WEATHER_ON', true);
    // Check if the LOCATION field has been set
    if (claySettings[messageKeys.LOCATION] &&
        claySettings[messageKeys.LOCATION] != '') {
      localStorage.setItem('LOCATION', claySettings[messageKeys.LOCATION]);
    } else {
      localStorage.setItem('LOCATION', null);
    }

    if (claySettings[messageKeys.UNITS] == 1) {
      localStorage.setItem('UNITS', claySettings[messageKeys.UNITS]);
    } else {
      localStorage.setItem('UNITS', null);
    }

    getWeather();
  } else {
    // Send settings values to watch side
    Pebble.sendAppMessage(
        dict,
        function(e) {
          // console.log('Sent config data to Pebble');
        },
        function(e) {
          console.log('Failed to send config data!');
          console.log(JSON.stringify(e));
        });
  }
});
