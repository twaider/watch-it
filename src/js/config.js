module.exports = [
  {
    "type": "heading",
    "defaultValue": "WatchIt"
  },
  {
    "type": "text",
    "defaultValue": "Configuration"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },      
      {
        "type": "toggle",
        "messageKey": "BACKGROUND_ON",
        "label": "Enable Custom Hours Color",
        "defaultValue": false
      },
      {
        "type": "color",
        "messageKey": "BACKGROUND_COLOR",
        "defaultValue": "0x0055FF",
        "label": "Background Color"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "weather Settings"
      },
      {
        "type": "toggle",
        "messageKey": "WEATHER_ON",
        "label": "Enable Weather",
        "defaultValue": false
      },
      {
        "type": "toggle",
        "messageKey": "WEATHER_SAFEMODE",
        "label": "Enable Battery Saver",
        "defaultValue": true
      },
      {
        "type": "text",
        "defaultValue": "Battery Saver will pause weather updates during the night (between 00:00 and 06:00)",
      },
      {
        "type": "toggle",
        "messageKey": "UNITS",
        "label": "Use Fahrenheit (F)?",
        "defaultValue": false
      },
      {
        "type": "input",
        "messageKey": "LOCATION",
        "defaultValue": "",
        "label": "Manual location",
        "attributes": {
          "placeholder": "eg: London, UK",
          "type": "text"
        }
      },
      {
        "type": "text",
        "defaultValue": "Input your city name and country for better results, eg: 'London, GB'. Leave empty to use location service detect automatically",
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];