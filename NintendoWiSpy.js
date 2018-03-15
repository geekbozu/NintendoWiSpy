window.onload = function(){
    var theme = 'theme\\' + getAllUrlParams().theme + '\\',
        config = JSON.parse(getJSON(theme + 'config.json')),
        canvas = document.getElementById('myCanvas'),
        ctx = canvas.getContext('2d'),
        heartbeatMsg = 'pong',
        heartbeatInterval = null,
        missedHeartbeats = 0,
        i,
        RSSI = null,
        socket = new ReconnectingWebsocket('ws://' + getAllUrlParams().websocketserver + ':' + getAllUrlParams().websockport);

    if(config.width){
        canvas.width = config.width;
    }
    if(config.height){
        canvas.height = config.height;
    }
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.textAlign = "center";
    ctx.fillText('Waiting for connection', canvas.width / 2, canvas.height / 2);

    socket.onopen = function(event){
        console.log('Connected to: ' + event.currentTarget.url);
        if(heartbeatInterval === null){
            missedHeartbeats = 0;
            heartbeatInterval = setInterval(function(){
                try{
                    missedHeartbeats++;
                    if(missedHeartbeats >= 4){
                        throw new Error('Too many missed heartbeats.');
                    }
                    socket.send('ping');
                } catch(e){
                    clearInterval(heartbeatInterval);
                    heartbeatInterval = null;
                    ctx.clearRect(0, 0, canvas.width, canvas.height);
                    ctx.textAlign = 'center';
                    ctx.fillText('Waiting for connection', canvas.width/2, canvas.height/2);
                    console.warn('Closing connection. Reason: ' + e.message);
                    socket.close();
                }
            }, 5000);
        }
    };
    window.onunload = function(){
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
    if(config.static){
        for(i in config.static){
            config.static[i].img = new Image();
            config.static[i].img.src = theme + config.static[i].file;
        }
    }

    socket.onmessage = function(event){
        // console.log('Recieved: ' + event.data);
        var controlsObj = extractControls(event.data),
            i,
            x,
            y,
            sy,
            sx,
            sheight,
            swidth;

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

        switch(config.controllerType){
            case'GCN':
            case'N64':
                for(i in config.button){
                    if(controlsObj.button[i]){
                        if(config.button[i].width && config.button[i].height){
                            ctx.drawImage(config.button[i].img, config.button[i].x, config.button[i].y, config.button[i].width, config.button[i].height);
                        }else{
                            ctx.drawImage(config.button[i].img, config.button[i].x, config.button[i].y);
                        }
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

                    switch(config.analog[i].direction){ // SubX SubY Starting point
                        case'up':
                            sx = 0;
                            sy = config.analog[i].img.height - sheight;
                            y += sy;
                            sheight = controlsObj.analog[config.analog[i].axis] * config.analog[i].img.height;
                            swidth = config.analog[i].img.width;
                            break;
                        case'down':
                            sx = 0;
                            sy = 0;
                            swidth = config.analog[i].img.width;
                            sheight = controlsObj.analog[config.analog[i].axis] * config.analog[i].img.height;
                            break;
                        case'left':
                            sy = 0;
                            swidth = controlsObj.analog[config.analog[i].axis] * config.analog[i].img.width;
                            sheight = config.analog[i].img.height;
                            sx = config.analog[i].img.width - swidth;
                            x += config.analog[i].img.width;
                            x -= swidth;
                            break;
                        default:
                        case'right':
                            sx = 0;
                            sy = 0;
                            sheight = config.analog[i].img.height;
                            swidth = controlsObj.analog[config.analog[i].axis] * config.analog[i].img.width;
                            break;
                    }
                    ctx.drawImage(config.analog[i].img, sx, sy, swidth, sheight, x, y, swidth, sheight);
                }
                break;
            default:
                console.log('No Controller type');
        }
        if(RSSI != null && config.WiFiStatus){
            ctx.font = config.WiFiStatus.height + ' Calibri';
            ctx.textAlign = 'left';
            ctx.textBaseline = 'top';
            ctx.fillStyle = ~~RSSI < -50 ? 'RGBA(255, 0, 0, 0.7)' : 'RGBA(0, 255, 0, 0.7)';
            ctx.fillText('RSSI:'+RSSI+'dBm', config.WiFiStatus.x,config.WiFiStatus.y);
        }
    };
    function extractControls(data){
        var button = {},
            analog = {};
        // Eeems says do this https://www.irccloud.com/pastebin/zvPElZLJ/
        switch(config.controllerType){
            case'GCN':
                button.START = Boolean(~~data[3]);
                button.A = Boolean(~~data[7]);
                button.B = Boolean(~~data[6]);
                button.X = Boolean(~~data[5]);
                button.Y = Boolean(~~data[4]);
                button.Z = Boolean(~~data[11]);
                button.L = Boolean(~~data[9]);
                button.R = Boolean(~~data[10]);
                // Todo analog shennangins

                analog.joyStickX = (getMultiByte(data, 16) - 128) / 128;
                analog.joyStickY = (getMultiByte(data, 16 + 8) - 128) / 128;
                analog.cStickX = (getMultiByte(data, 16 + 16) - 128) / 128;
                analog.cStickY = (getMultiByte(data, 16 + 24) - 128) / 128;
                analog.lTrig = getMultiByte(data, 16 + 32) / 256;
                analog.rTrig = getMultiByte(data, 16 + 40) / 256;
                break;

            case'N64':
                button.START = Boolean(~~data[3]);
                button.A = Boolean(~~data[0]);
                button.B = Boolean(~~data[1]);
                button.Z = Boolean(~~data[2]);
                button.L = Boolean(~~data[10]);
                button.R = Boolean(~~data[11]);
                button.up = Boolean(~~data[4]);
                button.down = Boolean(~~data[5]);
                button.left = Boolean(~~data[6]);
                button.right = Boolean(~~data[7]);
                button.Cup = Boolean(~~data[12]);
                button.Cdown = Boolean(~~data[13]);
                button.Cleft = Boolean(~~data[14]);
                button.Cright = Boolean(~~data[15]);

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
                break;
            default:
                console.log('No Controller type');
        }
        return {
            button: button,
            analog: analog
        };
    }
    /*            byte val = 0;
            for (int i = 0 ; i < 8 ; ++i) {
                if ((packet[i+offset] & 0x0F) != 0) {
                    val |= (byte)(1<<(7-i));
                }
            }
            ret
*/
    function getMultiByte(data, offset){
        var val, j = 0;
        for(j = 0; j < 8; ++j){
            if(data[j + offset] == '1'){
                val |= 1 << (7 - j);
            }
        }
        return val;
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

    // Stack overflow to the rescue
    // needs replacment eventually "deprecated" Bah
    function getJSON(url){
        var resp, xmlHttp;
        resp = '';
        xmlHttp = new XMLHttpRequest();

        if(xmlHttp != null){
            xmlHttp.open('GET', url, false);
            xmlHttp.send(null);
            resp = xmlHttp.responseText;
        }

        return resp;
    };
    function noop(){}
    function heartbeat(){
        this.isAlive = true;
    };
};
