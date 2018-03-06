# NintendoSpy Web Edition

A live input-viewer for GameCube/N64 controllers over wifi or usb serial. Used for showing/recording inputs during livestreams/gameplay recording sessions. 

# Documentation

## Features
- Runs on Windows/Linux/Mac
- Supports OG NintendoSpy Hardware
- Supports ESP8266 Hardware with wifi/Serial support
- Custom Themes in a JSON format

## Python Interface
Currently the python2.7 interface is built on top of TKinter. It is a means of starting a web server to host, the input viewer software (javascript + websockets), A Serial -> websocket bridge for older NintendoSpy hardware, and wifi credential management for the ESP2866 hardware.

## THEMES
NintendoSpy Web Edition Supports themes in a relatively simple JSON format
```
{
  "controllerType": "GCN",
  "theme": "Canadian Gamecube",
  "author": "Geekboy1011",
  "width": 200,
  "height": 50,
  "button": {
    "A": {
      "x": 35,
      "y": 21,
      "file": "A.png"
    }
  }, 
  "stick": {
    "joyStick": {
        "xname": "joyX",
        "yname": "joyY",
        "file": "Stick.png",
        "x": 14,
        "y": 29,
        "xrange": 16,
        "yrange": 16,
        "xreverse": false,
        "yreverse": false
    }
  },
  "analog": {
    "lTrig": {
        "axis": "lTrig",
        "file": "L.png",
        "x": 71,
        "y": 21,
        "direction": "left"
    }
  },
  "static": {
    "background": {
        "x": 0,
        "y": 0,
        "file": "back.png"
    }
  }
}
```
### Theme Objects:  
#### Root:
- controllerType: GCN | N64  
- Theme: Theme Name
- Author: Author
- width: Width of theme
- height: height of theme

#### button:
##### Available Buttons GCN
- A B X Y L R START Z  

##### Available Buttons N64
- A B L R START up down left right Cup Cleft Cright Cdown

##### arguments:
- x: X Coordinate in theme
- y: Y coordinate in theme
- file: Image file name
- width: Width to scale image to (optional)
- height: height to scale image to (optional)
#### stick:
##### Available stick GCN:
- joyStick cStick  

##### Available stick N64:
- joyStick
##### Available Axis
- stickname + X or Y
- EG: joyStickX, joyStickY

##### arguments:
- xname: joystick axis
- yname: joystick axis
- file: Image name
- x: X Coordinates
- y: Y Coordinate
- xrange: Pixes to move over stick X range
- yrange: Pixels to move over stick Y range
- xreverse: Reverse X axis
- yreverse: Reverse Y Axis
#### analog:
##### Available analog GCN:
- LTrig rTrig
##### Available analog N64
- NONE
##### available Axis
- See stick axis + available analog
##### arguments:
- axis: Axis
- file: Image Name
- x: X Coord
- y: Y Coord
- direction: Direction to sweep image
#### static:
##### arguments:
- x: X Coordinate
- y: Y Coordinate
- file: Image Name

### Hardware
TO DOCUMENT 
