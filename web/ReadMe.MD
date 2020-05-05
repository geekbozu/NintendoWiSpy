# Web Page for NintendoWiSpy

## Install

```shell
npm install
npm browserify NintendoWiSpy.js -o index.js
```

## Themes

Themes are located in theme\
See included themes for fully functional examples

```
{
  "controllerType": "GCN",          Controller Type
  "theme": "Canadian Gamecube",     Theme Name
  "author": "Geekboy1011",          Theme Author
  "width": 200,                     Width of theme
  "height": 50,                     Height of Theme
  "inputdelay": 900,                Input Delay in MS to match video
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
  },
  "rangeButton":{                   Range Button Object
    "ess":{                         name
      "file": "ESS.png",            Image file
      "x": 0,                       X Coordinate
      "y": 16,                      Y Coordinate
      "axis":{                      Object containing axis to check
        "joyStickX":{               First Axis name
          "range1":{                range1..rangeN
            "from": 0.21875,        From what position
            "to": 0.2890625         to what position
          },
          "range2":{
            "from": -0.3046875,     negatives go from lowest
            "to": -0.2578125        To highest
          }
        },
        "joyStickY":{               Second axis
          "range1":{
            "from": 0.1953125,
            "to": 0.265625
          },
          "range2":{
            "from": -0.3671875,
            "to": -0.296875
          }
        }
      }
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
- inputdelay: Render Delay in MS

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

##### Available stick :

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

### rangeButton:

Shows an image if an analog or stick is within the specified bounds

##### available Axis

- LTrig rTrig joyStickX joyStickY cStickX cStickY

### static:

Displays a static image.