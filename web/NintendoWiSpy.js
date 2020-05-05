(function () {
    async function getJSON(url) {
        const res = await fetch(url);
        return await res.json();
    }
    function heartbeat() {
        this.isAlive = true;
    }
    function getMultiByte(data, offset) {
        var val, j = 0;
        for (j = 0; j < 8; ++j) {
            if (data[j + offset] == '1') {
                val |= 1 << (7 - j);
            }
        }
        return val;
    }
    function drawStrokeText(ctx, text, x, y,width) {
        ctx.font = 'Calibri';
        ctx.miterLimit = 2;
        ctx.textAlign = 'center';
        ctx.strokeStyle = 'black';
        ctx.lineWidth = 4;
        ctx.strokeText(text, x, y,width);
        ctx.fillStyle = 'white';
        ctx.fillText(text, x, y, width);
    }
    // Graciously stolen from sitepoint
    // https://www.sitepoint.com/get-url-parameters-with-javascript/
    function getAllUrlParams(url) {
        // get query string from url (optional) or window
        var queryString = url ? url.split('?')[1] : window.location.search.slice(1);

        // we'll store the parameters here
        var obj = {};

        // if query string exists
        if (queryString) {
            // stuff after # is not part of query string, so get rid of it
            queryString = queryString.split('#')[0];

            // split our query string into its component parts
            var arr = queryString.split('&');

            for (var i = 0; i < arr.length; i++) {
                // separate the keys and the values
                var a = arr[i].split('=');

                // in case params look like: list[]=thing1&list[]=thing2
                var paramNum;
                var paramName = a[0].replace(/\[\d*\]/, function (v) {
                    paramNum = v.slice(1, -1);
                    return '';
                });

                // set parameter value (use 'true' if empty)
                var paramValue = typeof (a[1]) === 'undefined' ? true : a[1];

                // (optional) keep case consistent
                paramName = paramName.toLowerCase();
                paramValue = paramValue.toLowerCase();

                // if parameter name already exists
                if (obj[paramName]) {
                    // convert value to array (if still string)
                    if (typeof obj[paramName] === 'string') {
                        obj[paramName] = [obj[paramName]];
                    }
                    // if no array index number specified...
                    if (typeof paramNum === 'undefined') {
                        // put the value on the end of the array
                        obj[paramName].push(paramValue);
                    }
                    // if array index number specified...
                    else {
                        // put the value at that index number
                        obj[paramName][paramNum] = paramValue;
                    }
                }
                // if param name doesn't exist yet, set it
                else {
                    obj[paramName] = paramValue;
                }
            }
        }
        return obj;
    }
    window.onload = async () => {
        const ReconnectingWebSocket = require('reconnecting-websocket'),
            params = getAllUrlParams(),
            theme = 'theme\\' + params.theme + '\\',
            config = await getJSON(theme + 'config.json'),
            canvas = document.getElementById('myCanvas'),
            ctx = canvas.getContext('2d'),
            heartbeatMsg = 'pong',
            socket = new ReconnectingWebSocket('ws://' + params.websocketserver + ':' + params.websockport);
        var heartbeatInterval = null,
            missedHeartbeats = 0,
            ginputdelay = 0,
            i,
            RSSI = null,
            xofs = 0,
            yofs = 0,
            cyofs = 0,
            cxofs = 0,
            lofs = 0,
            rofs = 0,
            zero = true,
            buttonCount = {}
            branch = drawInputView;

        if (config.width) {
            canvas.width = config.width;
        }
        if (config.height) {
            canvas.height = config.height;
        }
        if (params.inputdelay) {
            ginputdelay = params.inputdelay;
        }
        if (params.view)
        {
            if (params.view.toLowerCase() == "count")
               branch = drawCountView;
            if (params.view.toLowerCase() == "input")
               branch = drawInputView;
        }
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        drawStrokeText(ctx, 'Waiting for Connection.', canvas.width / 2, canvas.height / 2, canvas.width);

        socket.onopen = (event) => {
            console.log('Connected to: ' + event.currentTarget.url);
            if (heartbeatInterval === null) {
                missedHeartbeats = 0;
                heartbeatInterval = setInterval(() => {
                    try {
                        missedHeartbeats++;
                        if (missedHeartbeats >= 4) {
                            throw new Error('Too many missed heartbeats.');
                        }
                        socket.send('ping');
                    } catch (e) {
                        clearInterval(heartbeatInterval);
                        heartbeatInterval = null;
                        ctx.clearRect(0, 0, canvas.width, canvas.height);
                        drawStrokeText(ctx, 'Waiting for Connection.', canvas.width / 2, canvas.height / 2, canvas.width);
                        console.warn('Closing connection. Reason: ' + e.message);
                        //socket.reconnect();
                    }
                }, 5000);
            }
        };
        window.onunload = () => {
            socket.close();
        };
        config.WiFiStatus.badimg = new Image();
        config.WiFiStatus.badimg.src = theme + config.WiFiStatus.bad;
        config.WiFiStatus.okimg = new Image();
        config.WiFiStatus.okimg.src = theme +config.WiFiStatus.good;
        config.WiFiStatus.goodimg = new Image();
        config.WiFiStatus.goodimg.src = theme + config.WiFiStatus.good;
        
        if (config.inputView.button) {
            for (i in config.inputView.button) {
                config.inputView.button[i].img = new Image();
                config.inputView.button[i].img.src = theme + config.inputView.button[i].file;
            }
        }
        if (config.inputView.analog) {
            for (i in config.inputView.analog) {
                config.inputView.analog[i].img = new Image();
                config.inputView.analog[i].img.src = theme + config.inputView.analog[i].file;
            }
        }
        if (config.inputView.stick) {
            for (i in config.inputView.stick) {
                config.inputView.stick[i].img = new Image();
                config.inputView.stick[i].img.src = theme + config.inputView.stick[i].file;
            }
        }
        if (config.inputView.rangeButton) {
            for (i in config.inputView.rangeButton) {
                config.inputView.rangeButton[i].img = new Image();
                config.inputView.rangeButton[i].img.src = theme + config.inputView.rangeButton[i].file;
            }
        }
        if (config.inputView.static) {
            for (i in config.inputView.static) {
                config.inputView.static[i].img = new Image();
                config.inputView.static[i].img.src = theme + config.inputView.static[i].file;
            }
        }
        if (config.countView.button) {
            for (i in config.countView.button) {
                config.countView.button[i].img = new Image();
                config.countView.button[i].img.src = theme + config.countView.button[i].file;
            }
        }
        if (config.countView.static) {
            for (i in config.countView.static) {
                config.countView.static[i].img = new Image();
                config.countView.static[i].img.src = theme + config.countView.static[i].file;
            }
        }
        const GCN_N64_Direction = {
            up: (controlsObj, controlConf, x, y, sy, sx, sheight, swidth) => {
                sheight = controlsObj.analog[controlConf.axis] * controlConf.img.height;
                sx = 0;
                sy = controlConf.img.height - sheight;
                y += sy;
                swidth = controlConf.img.width;
                return [x, y, sy, sx, sheight, swidth];
            },
            down: (controlsObj, controlConf, x, y, sy, sx, sheight, swidth) => {
                sx = 0;
                sy = 0;
                swidth = controlConf.img.width;
                sheight = controlsObj.analog[controlConf.axis] * controlConf.img.height;
                return [x, y, sy, sx, sheight, swidth];
            },
            left: (controlsObj, controlConf, x, y, sy, sx, sheight, swidth) => {
                sy = 0;
                swidth = controlsObj.analog[controlConf.axis] * controlConf.img.width;
                sheight = controlConf.img.height;
                sx = controlConf.img.width - swidth;
                x += controlConf.img.width;
                x -= swidth;
                return [x, y, sy, sx, sheight, swidth];
            },
            right: (controlsObj, controlConf, x, y, sy, sx, sheight, swidth) => {
                sx = 0;
                sy = 0;
                sheight = controlConf.img.height;
                swidth = controlsObj.analog[controlConf.axis] * controlConf.img.width;
                return [x, y, sy, sx, sheight, swidth];
            }
        };
        initButtons(buttonCount);
        socket.onmessage = (event) => {
            // console.log('Recieved: ' + event.data);
            let controlsObj,
                i,
                d = '';
            switch (event.data) {
                case heartbeatMsg:
                    // reset the counter for missed heartbeats
                    console.log('Recieved: ' + event.data);
                    missedHeartbeats = 0;
                    return;
                case 'ZERO':
                    zero = true;
                    return;
                case 'REFRESH':
                    window.location.reload(true);
                    return;
                case 'CLEAR':
                    initButtons(buttonCount);
                    return;
                case 'COUNTS':
                    if (params.view == null)
                    {
                        if (branch == drawInputView)
                            branch = drawCountView;
                        else
                            branch = drawInputView;
                    }
                    return;
                case 'Connected':
                    return;
            }
            
            if (event.data.slice(0, 1) === 'B') {
                console.log(event.data);
                return;
            }
            if (event.data.slice(0, 1) === 'b') {
                console.log(event.data);
                return;
            }
            if (event.data.slice(0, 4) === 'RSSI') {
                RSSI = event.data.slice(5);
                console.log("RSSI:" + RSSI);
                return;
            }
            if (event.data.charCodeAt(0) <= 1) {
                var char = String.fromCharCode(0);
                for (i of event.data) {
                    d += (i === char) ? '0' : '1';
                }
            } else {
                d = event.data;
            }
            controlsObj = extractControls(d);
             //If we have last
            if("last" in buttonCount){ //if buttoncount.last exists


                switch (config.controllerType){
                    case 'GCN':
                        if (params.view == null){
                            if (!(controlsObj.button.A && controlsObj.button.B && controlsObj.button.Y && controlsObj.button.X)
                               && (buttonCount.last.A && buttonCount.last.B && buttonCount.last.X && buttonCount.last.Y)){
                               if (branch == drawInputView)
                                   branch = drawCountView;
                               else
                                   branch = drawInputView;
                           }
                        }
                       break
                   case 'N64':
                       break
               }

               //Check input count
                for (i in controlsObj.button){   
                    if(buttonCount.last[i] == true && controlsObj.button[i] == false){  //If button has been released
                        console.log(i,buttonCount[i]);
                        buttonCount[i] += 1;
                    }
                }
            }
            buttonCount.last = controlsObj.button;
            setTimeout(branch, ginputdelay, controlsObj);
        };

        function drawInputView(controlsObj) {
            requestAnimationFrame(() => {
                ctx.clearRect(0, 0, canvas.width, canvas.height);

                for (i in config.inputView.static) {
                    ctx.drawImage(config.inputView.static[i].img, config.inputView.static[i].x, config.inputView.static[i].y);
                }
                if (config.controllerType === 'GCN' || config.controllerType === 'N64') {
                    let x,
                        y;

                    for (i in config.inputView.button) {
                        if (controlsObj.button[i]) {
                            if (config.inputView.button[i].width && config.inputView.button[i].height) {
                                ctx.drawImage(config.inputView.button[i].img, config.inputView.button[i].x, config.inputView.button[i].y, config.inputView.button[i].width, config.inputView.button[i].height);
                            } else {
                                ctx.drawImage(config.inputView.button[i].img, config.inputView.button[i].x, config.inputView.button[i].y);
                            }
                        }
                    }
                    for (i in config.inputView.rangeButton) { // For each rangeButton
                        let max = true,
                            min = false;
                        for (let j in config.inputView.rangeButton[i].axis) { // for each axis type
                            for (let r in config.inputView.rangeButton[i].axis[j]) { // For each "range"
                                const from = Math.abs(config.inputView.rangeButton[i].axis[j][r].from),
                                    to = Math.abs(config.inputView.rangeButton[i].axis[j][r].to),
                                    pos = Math.abs(controlsObj.analog[j]);
                                if (pos > to) {
                                    max = false; // If Coordinates are outside max
                                }
                                if (pos >= from) {
                                    min = true; // If Coordinates are inside min
                                }
                                if (max && min) {
                                    break; // If both are in we can move to next axis
                                }
                            }
                            if (!max) {
                                break; // If anything was over max we are done
                            }
                        }
                        if (max && min) {
                            ctx.drawImage(config.inputView.rangeButton[i].img, config.inputView.rangeButton[i].x, config.inputView.rangeButton[i].y);
                        }
                    }
                    for (i in config.inputView.stick) {
                        if (config.inputView.stick[i].yname in controlsObj.analog && config.inputView.stick[i].xname in controlsObj.analog) { // If the axis exist
                            x = ((config.inputView.stick[i].xreverse ? -1 : 1) * config.inputView.stick[i].xrange);
                            y = ((config.inputView.stick[i].yreverse ? 1 : -1) * config.inputView.stick[i].yrange);
                            x *= controlsObj.analog[config.inputView.stick[i].xname];
                            y *= controlsObj.analog[config.inputView.stick[i].yname];
                            x += config.inputView.stick[i].x;
                            y += config.inputView.stick[i].y;
                            ctx.drawImage(config.inputView.stick[i].img, x, y);
                        }
                    }
                    for (i in config.inputView.analog) {
                        let sy,
                            sx,
                            sheight,
                            swidth;
                        x = config.inputView.analog[i].x;
                        y = config.inputView.analog[i].y;// Location on screen
                        let direction = config.inputView.analog[i].direction;
                        if (direction in GCN_N64_Direction) {
                            [x, y, sy, sx, sheight, swidth] = GCN_N64_Direction[config.inputView.analog[i].direction](controlsObj, config.inputView.analog[i], x, y, sy, sx, sheight, swidth);
                        }
                        ctx.drawImage(config.inputView.analog[i].img, sx, sy, swidth, sheight, x, y, swidth, sheight);
                    }
                } else {
                    console.log('No Controller type');
                }

                if (RSSI != null && config.WiFiStatus) {
                    //console.log("RSSI:" + RSSI);
                    if(RSSI < -60){
                        ctx.drawImage(config.WiFiStatus.goodimg,config.WiFiStatus.x,config.WiFiStatus.y);
                    } else if (RSSI < -50) {
                        ctx.drawImage(config.WiFiStatus.okimg,config.WiFiStatus.x,config.WiFiStatus.y);
                    } else {
                        ctx.drawImage(config.WiFiStatus.badimg,config.WiFiStatus.x,config.WiFiStatus.y);
                    }
                }
                
            });
        }
        function drawCountView(controlsObj) {
            requestAnimationFrame( () => {
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                for (i in config.countView.static) {
                    ctx.drawImage(config.countView.static[i].img, config.countView.static[i].x, config.countView.static[i].y);
                }
                if (config.controllerType === 'GCN' || config.controllerType === 'N64') {
                    let x,
                        y;

                    for (i in config.countView.button) {
                        if (config.countView.button[i].width && config.countView.button[i].height) {
                            ctx.drawImage(config.countView.button[i].img, config.countView.button[i].x, config.countView.button[i].y, config.countView.button[i].width, config.countView.button[i].height);
                        } else {
                            ctx.drawImage(config.countView.button[i].img, config.countView.button[i].x, config.countView.button[i].y);
                        }
                        ctx.font = config.countView.font;
                        ctx.fillStyle = 'White';
                        ctx.fillText(buttonCount[i], config.countView.button[i].img.width+config.countView.button[i].x, (config.countView.button[i].img.height/2)+config.countView.button[i].y,100);
                    }
                }
            });
        }
        const controls = {
            GCN: (data, button, analog) => {
                button.START = !!~~data[3];
                button.A = !!~~data[7];
                button.B = !!~~data[6];
                button.X = !!~~data[5];
                button.Y = !!~~data[4];
                button.Z = !!~~data[11];
                button.L = !!~~data[9];
                button.R = !!~~data[10];

                analog.joyStickX = ((getMultiByte(data, 16) - 128) / 128) - xofs;
                analog.joyStickY = ((getMultiByte(data, 16 + 8) - 128) / 128) - yofs;
                analog.cStickX = ((getMultiByte(data, 16 + 16) - 128) / 128) - cxofs;
                analog.cStickY = ((getMultiByte(data, 16 + 24) - 128) / 128) - cyofs;
                analog.lTrig = (getMultiByte(data, 16 + 32) / 256) - lofs;
                analog.rTrig = (getMultiByte(data, 16 + 40) / 256) - rofs;
                if (zero) { // If we have to zero axis
                    xofs = (getMultiByte(data, 16) - 128) / 128;
                    yofs = (getMultiByte(data, 16 + 8) - 128) / 128;
                    cxofs = (getMultiByte(data, 16 + 16) - 128) / 128;
                    cyofs = (getMultiByte(data, 16 + 24) - 128) / 128;
                    lofs = getMultiByte(data, 16 + 32) / 256;
                    rofs = getMultiByte(data, 16 + 40) / 256;
                    zero = false;
                }
            },
            N64: (data, button, analog) => {
                button.START = !!~~data[3];
                button.A = !!~~data[0];
                button.B = !!~~data[1];
                button.Z = !!~~data[2];
                button.L = !!~~data[10];
                button.R = !!~~data[11];
                button.up = !!~~data[4];
                button.down = !!~~data[5];
                button.left = !!~~data[6];
                button.right = !!~~data[7];
                button.Cup = !!~~data[12];
                button.Cdown = !!~~data[13];
                button.Cleft = !!~~data[14];
                button.Cright = !!~~data[15];

                analog.joyStickX = (~~getMultiByte(data, 16));
                analog.joyStickY = (~~getMultiByte(data, 16 + 8));
                if (analog.joyStickX > 127) {
                    analog.joyStickX -= 256;
                }
                if (analog.joyStickY > 127) {
                    analog.joyStickY -= 256;
                }
                analog.joyStickX /= 128;
                analog.joyStickY /= 128;
                if (zero) { // If we have to zero axis
                    xofs = analog.joyStickX;
                    yofs = analog.joyStickY;
                    zero = false;
                }
                analog.joyStickX -= xofs;
                analog.joyStickY -= yofs;
            }
        };
        function initButtons(dict) {
            dict.A = 0;
            dict.B = 0;
            dict.X = 0;
            dict.Y = 0;
            dict.R = 0;
            dict.L = 0;
            dict.Z = 0;
            dict.START = 0;
            dict.left = 0;
            dict.right = 0;
            dict.down = 0;
            dict.Cup = 0;
            dict.Cdown = 0;
            dict.Cleft = 0;
            dict.Cright = 0;
        }
        function extractControls(data) {
            var button = {},
                analog = {};
            if (config.controllerType in controls) {
                controls[config.controllerType](data, button, analog);
            } else {
                console.log('No Controller type');
            }
            return { button, analog };
        }
    };
})();
