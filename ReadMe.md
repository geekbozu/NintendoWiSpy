# NintendoSpy Web Edition

A live input-viewer for GameCube/N64 controllers over wifi or usb serial. Used for showing/recording inputs during livestreams/gameplay recording sessions.

# Documentation

## Features
- Runs on Windows/Linux/Mac
- Supports OG NintendoSpy Hardware
- Supports ESP8266 Hardware with wifi/Serial support
- Custom Themes in a JSON format
## Requirements
- Python2.7
- PySerial
- [python-websocket-server](https://github.com/Pithikos/python-websocket-server)
## Python Interface
Currently the python2.7 interface is built on top of TKinter. It is a means of starting a web server to host, the input viewer software (javascript + websockets), A Serial -> websocket bridge for older NintendoSpy hardware, and wifi credential management for the ESP2866 hardware.
![](https://i.imgur.com/SqLejSL.png)
##### Serial Baudrate:
- Baud Rate for connected serial device.
##### HTTPSERVER port:
- Port to host the http server on.
##### WebSocket Host:
- Host of the websocket server.
##### Display Socket Port:
- Port that the display applet listens on for controller information.
##### Serial Port:
- Available Serial Devices
##### Theme
- Available Themes.
##### httpserver button:
- Starts a http server to serve the display applet.
##### Serial Forwarder:
- Starts a serial forwarder based off selected comport, Forwards serial packets to display applet.
##### Generate Url:
- Copies Display applet url to your clip board
## THEMES
NintendoSpy Web Edition Supports themes in a relatively simple JSON format
Themes are located in theme\
See included themes for fully functional examples
```
{
  "controllerType": "GCN",          Controller Type
  "theme": "Canadian Gamecube",     Theme Name
  "author": "Geekboy1011",          Theme Author
  "width": 200,                     Width of theme
  "height": 50,                     Height of Theme
  "WiFiStatus": {                   Optional Struct
    "height": "10px",               WiFi status message height
    "x": 0,                         X
    "y": 0                          Y
  },
  "button": {                       Button Object
    "A": {                          Button
      "x": 35,                      X Coordinate
      "y": 21,                      Y Coordinate
      "file": "A.png",              Image File Nam
      "width": 10,                  Scale image to width (Optional)
      "height": 10                  Scale image to height (Optional)
    }
  },
  "stick": {                        Stick Object
    "joyStick": {                   Stick name
        "xname": "joyX",            X Axis
        "yname": "joyY",            Y Axis
        "file": "Stick.png",        Image File Name
        "x": 14,                    X Coordinate
        "y": 29,                    Y Coordinate
        "xrange": 16,               Range in pixels to move image over axis
        "yrange": 16,               Range in pixels to move image over axis
        "xreverse": false,          Reverse X axis
        "yreverse": false           Reverse Y axis
    }
  },
  "analog": {                       Analog Object
    "lTrig": {                      name
        "axis": "lTrig",            Axis
        "file": "L.png",            Image File Name
        "x": 71,                    X Coordinate
        "y": 21,                    Y Coordinate
        "direction": "left"         Image Render Direction
    }
  },
  "static": {                       Static Object
    "background": {                 Name
        "x": 0,                     X coordinate
        "y": 0,                     Y Coordinate
        "file": "back.png"          Image File Name
    }
  }
}
```
## Theme Objects:
### Root:
Stores global information for the theme.
- controllerType: GCN | N64
- Theme: Theme Name
- Author: Author
- width: Width of theme
- height: height of theme

### WiFiStatus:
Optional
If present has the client display current wifi signal strength recieved from NintedoWiSpy
- height: Font height, Can be any browser unit
- x: X coordinate
- y: y coordinate of bottom of text
### button:
Buttons when pressed show the image file.
##### Available Buttons GCN
- A B X Y L R START Z

##### Available Buttons N64
- A B L R Z START up down left right Cup Cleft Cright Cdown

### stick:
Shows a image based off of the Stick Location.
##### Available stick GCN:
- joyStick cStick

##### Available stick N64:
- joyStick
##### Available Axis
- stickname + X or Y
- EG: joyStickX, joyStickY

### analog:
Shows an image starting from "direction" based on Analog position
##### Available analog GCN:
- LTrig rTrig
##### Available analog N64
- NONE
##### available Axis
- See stick axis + available analog
### static:
Displays a static image.

### Hardware
TO DOCUMENT
