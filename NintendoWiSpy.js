(function(){
    async function getJSON(url){
        const res = await fetch(url);
        return await res.json();
    }
    function heartbeat(){
        this.isAlive = true;
    }
    function getMultiByte(data, offset){
        var val, j = 0;
        for(j = 0; j < 8; ++j){
            if(data[j + offset] == '1'){
                val |= 1 << (7 - j);
            }
        }
        return val;
    }
    function drawStrokeText(ctx, text, x, y){
        ctx.font = 'Calibri';
        ctx.miterLimit = 2;
        ctx.textAlign = 'center';
        ctx.strokeStyle = 'black';
        ctx.lineWidth = 4;
        ctx.strokeText(text, x, y, x * 2);
        ctx.fillStyle = 'white';
        ctx.fillText(text, x, y, x * 2);
    }
    // Graciously stolen from sitepoint
    // https://www.sitepoint.com/get-url-parameters-with-javascript/
    function getAllUrlParams(url){
        // get query string from url (optional) or window
        var queryString = url ? url.split('?')[1] : window.location.search.slice(1);

        // we'll store the parameters here
        var obj = {};

        // if query string exists
        if(queryString){
            // stuff after # is not part of query string, so get rid of it
            queryString = queryString.split('#')[0];

            // split our query string into its component parts
            var arr = queryString.split('&');

            for(var i = 0; i < arr.length; i++){
                // separate the keys and the values
                var a = arr[i].split('=');

                // in case params look like: list[]=thing1&list[]=thing2
                var paramNum;
                var paramName = a[0].replace(/\[\d*\]/, function(v){
                    paramNum = v.slice(1, -1);
                    return '';
                });

                // set parameter value (use 'true' if empty)
                var paramValue = typeof (a[1]) === 'undefined' ? true : a[1];

                // (optional) keep case consistent
                paramName = paramName.toLowerCase();
                paramValue = paramValue.toLowerCase();

                // if parameter name already exists
                if(obj[paramName]){
                    // convert value to array (if still string)
                    if(typeof obj[paramName] === 'string'){
                        obj[paramName] = [obj[paramName]];
                    }
                    // if no array index number specified...
                    if(typeof paramNum === 'undefined'){
                        // put the value on the end of the array
                        obj[paramName].push(paramValue);
                    }
                    // if array index number specified...
                    else{
                    // put the value at that index number
                        obj[paramName][paramNum] = paramValue;
                    }
                }
                // if param name doesn't exist yet, set it
                else{
                    obj[paramName] = paramValue;
                }
            }
        }
        return obj;
    }
    window.onload = async () => {
        const params = getAllUrlParams(),
            theme = 'theme\\' + params.theme + '\\',
            config = await getJSON(theme + 'config.json'),
            canvas = document.getElementById('myCanvas'),
            ctx = canvas.getContext('2d'),
            heartbeatMsg = 'pong',
            socket = new ReconnectingWebsocket('ws://' + params.websocketserver + ':' + params.websockport, 'arduino');
        var heartbeatInterval = null,
            missedHeartbeats = 0,
            i,
            RSSI = null;

        if(config.width){
            canvas.width = config.width;
        }
        if(config.height){
            canvas.height = config.height;
        }
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        drawStrokeText(ctx, 'Waiting for Connection.', canvas.width / 2, canvas.height / 2);
        socket.onopen = (event) => {
            console.log('Connected to: ' + event.currentTarget.url);
            if(heartbeatInterval === null){
                missedHeartbeats = 0;
                heartbeatInterval = setInterval(() => {
                    try{
                        missedHeartbeats++;
                        if(missedHeartbeats >= 4){
                            throw new Error('Too many missed heartbeats.');
                        }
                        socket.send('ping');
                    }catch(e){
                        clearInterval(heartbeatInterval);
                        heartbeatInterval = null;
                        ctx.clearRect(0, 0, canvas.width, canvas.height);
                        drawStrokeText(ctx, 'Waiting for Connection.', canvas.width / 2, canvas.height / 2);
                        console.warn('Closing connection. Reason: ' + e.message);
                        socket.close();
                    }
                }, 5000);
            }
        };
        window.onunload = () => {
            socket.close();
        };

        if(config.button){
            for(i in config.button){
                config.button[i].img = new Image();
                config.button[i].img.src = theme + config.button[i].file;
            }
        }
        if(config.analog){
            for(i in config.analog){
                config.analog[i].img = new Image();
                config.analog[i].img.src = theme + config.analog[i].file;
            }
        }
        if(config.stick){
            for(i in config.stick){
                config.stick[i].img = new Image();
                config.stick[i].img.src = theme + config.stick[i].file;
            }
        }
        if(config.rangeButton){
            for(i in config.rangeButton){
                config.rangeButton[i].img = new Image();
                config.rangeButton[i].img.src = theme + config.rangeButton[i].file;
            }
        }
        if(config.static){
            for(i in config.static){
                config.static[i].img = new Image();
                config.static[i].img.src = theme + config.static[i].file;
            }
        }
        const GCN_N64_Direction = {
            up: (controlsObj, controlConf, x, y, sy, sx, sheight, swidth) => {
                sx = 0;
                sy = controlConf.img.height - sheight;
                y += sy;
                sheight = controlsObj.analog[controlConf.axis] * controlConf.img.height;
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

        socket.onmessage = (event) => {
            // console.log('Recieved: ' + event.data);
            let i;

            if(event.data == 'Connected'){
                return;
            }
            if(event.data === heartbeatMsg){
                // reset the counter for missed heartbeats
                console.log('Recieved: ' + event.data);
                missedHeartbeats = 0;
                return;
            }

            if(event.data.slice(0, 4) === 'RSSI'){
                RSSI = event.data.slice(5);
                return;
            }

            ctx.clearRect(0, 0, canvas.width, canvas.height);

            for(i in config.static){
                ctx.drawImage(config.static[i].img, config.static[i].x, config.static[i].y);
            }
            if(config.controllerType === 'GCN' || config.controllerType === 'N64'){
                let controlsObj = extractControls(event.data),
                    x,
                    y,
                    sy,
                    sx,
                    sheight,
                    swidth;
                for(i in config.button){
                    if(controlsObj.button[i]){
                        if(config.button[i].width && config.button[i].height){
                            ctx.drawImage(config.button[i].img, config.button[i].x, config.button[i].y, config.button[i].width, config.button[i].height);
                        }else{
                            ctx.drawImage(config.button[i].img, config.button[i].x, config.button[i].y);
                        }
                    }
                }
                for(i in config.rangeButton){ // For each rangeButton
                    let max = true,
                        min = false;
                    for(let j in config.rangeButton[i].axis){ // for each axis type
                        for(let r in config.rangeButton[i].axis[j]){ // For each "range"
                            const from = Math.abs(config.rangeButton[i].axis[j][r].from),
                                to = Math.abs(config.rangeButton[i].axis[j][r].to),
                                pos = Math.abs(controlsObj.analog[j]);
                            if(pos > to){
                                max = false; // If Coordinates are outside max
                            }
                            if(pos >= from){
                                min = true; // If Coordinates are inside min
                            }
                            if(max && min){
                                break; // If both are in we can move to next axis
                            }
                        }
                        if(!max){
                            break; // If anything was over max we are done
                        }
                    }
                    if(max && min){
                        ctx.drawImage(config.rangeButton[i].img, config.rangeButton[i].x, config.rangeButton[i].y);
                    }
                }
                for(i in config.stick){
                    if(config.stick[i].yname in controlsObj.analog && config.stick[i].xname in controlsObj.analog){ // If the axis exist
                        x = ((config.stick[i].xreverse ? -1 : 1) * config.stick[i].xrange);
                        y = ((config.stick[i].yreverse ? 1 : -1) * config.stick[i].yrange);
                        x *= controlsObj.analog[config.stick[i].xname];
                        y *= controlsObj.analog[config.stick[i].yname];
                        x += config.stick[i].x;
                        y += config.stick[i].y;
                        ctx.drawImage(config.stick[i].img, x, y);
                    }
                }
                for(i in config.analog){
                    x = config.analog[i].x;
                    y = config.analog[i].y;// Location on screen
                    let direction = config.analog[i].direction;
                    if(direction in GCN_N64_Direction){
                        [x, y, sy, sx, sheight, swidth] = GCN_N64_Direction[config.analog[i].direction](controlsObj, config.analog[i], x, y, sy, sx, sheight, swidth);
                    }
                    ctx.drawImage(config.analog[i].img, sx, sy, swidth, sheight, x, y, swidth, sheight);
                }

            }else{
                console.log('No Controller type');
            }
            if(RSSI != null && config.WiFiStatus){
                ctx.font = config.WiFiStatus.height + ' Calibri';
                ctx.textAlign = 'left';
                ctx.textBaseline = 'top';
                ctx.fillStyle = ~~RSSI < -50 ? 'RGBA(255, 0, 0, 0.7)' : 'RGBA(0, 255, 0, 0.7)';
                ctx.fillText('RSSI:' + RSSI + 'dBm', config.WiFiStatus.x, config.WiFiStatus.y);
            }

        };
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

                analog.joyStickX = (getMultiByte(data, 16) - 128) / 128;
                analog.joyStickY = (getMultiByte(data, 16 + 8) - 128) / 128;
                analog.cStickX = (getMultiByte(data, 16 + 16) - 128) / 128;
                analog.cStickY = (getMultiByte(data, 16 + 24) - 128) / 128;
                analog.lTrig = getMultiByte(data, 16 + 32) / 256;
                analog.rTrig = getMultiByte(data, 16 + 40) / 256;
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
                if(analog.joyStickX > 127){
                    analog.joyStickX -= 256;
                }
                if(analog.joyStickY > 127){
                    analog.joyStickY -= 256;
                }
                analog.joyStickX /= 128;
                analog.joyStickY /= 128;
            }
        };
        function extractControls(data){
            var button = {},
                analog = {};
            if(config.controllerType in controls){
                controls[config.controllerType](data, button, analog);
            }else{
                console.log('No Controller type');
            }
            return {button, analog};
        }
    };
})();
