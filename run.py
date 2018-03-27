import logging
import threading,sys,os,webbrowser,time
import serial,serial.tools.list_ports
import SimpleHTTPServer, BaseHTTPServer, SocketServer
from websocket_server import WebsocketServer
from Tkinter import *
import ttk,tkMessageBox

class ThreadingSimpleServer(SocketServer.ThreadingMixIn,BaseHTTPServer.HTTPServer):
    pass

httpServerRunning = False
serialServerRunning = False

def serialServerLoop():
    global serialServerRunning
    try:
        ser = serial.Serial(serialbox.get())
        ser.baudrate = SerialSpinBoxVar.get()
        ser.write('TS 1\n')
        ctime = time.time()
        while serialServerRunning:
            server.handle_request()
            line = ser.readline()
            server.handle_request()
            server.send_message_to_all(line)
            if (time.time() - ctime) > 4:
                ctime = time.time()
                server.send_message_to_all("pong")
        ser.write('TS 0\n')
        ser.close()
        server.server_close()
    except Exception as e:
        print "Failed to open"
        print e
        serialToggle()
        return

def serialToggle():
    global serialServerRunning,server
    if serialServerRunning:
        SerialButton.configure(bg='#F00')
        serialServerRunning = False
    else:
        server = WebsocketServer(WebSpinBoxVar.get(), host='127.0.0.1', loglevel=logging.INFO)
        server.set_fn_new_client(new_client)
        server.timeout = 0
        serialServerRunning = True
        serialthread = threading.Thread(target = serialServerLoop)
        serialthread.daemon = True
        serialthread.start()
        SerialButton.configure(bg='#0F0')

def httpToggle():
    global httpServerRunning
    global httpserver,httpthread
    if httpServerRunning:
        HttpButton.configure(bg='#F00')
        httpserver.shutdown()
        httpServerRunning = False
        os.chdir( sys.path[0] )
    else:
        HttpButton.configure(bg='#0F0')
        httpserver = ThreadingSimpleServer(('', HttpSpinBoxVar.get()), SimpleHTTPServer.SimpleHTTPRequestHandler)
        #os.chdir('theme\\' + themebox.get(themebox.curselection()))
        httpthread = threading.Thread(target = httpserver.serve_forever)
        httpthread.daemon = True
        httpthread.start()
        httpServerRunning = True

def new_client(client,server):
    print "New client gotten ", client

def genUrl():
    if not len(WebSockUrlEntryVar.get()):
        tkMessageBox.showerror("Url Error","WebSocketServer Url can not be blank!")
        return
    try:
        val = "http://localhost:{}/?theme={}&websockport={}&websocketserver={}".format(HttpSpinBoxVar.get(),themebox.get(),WebSpinBoxVar.get(),WebSockUrlEntryVar.get())
    except TclError:
        tkMessageBox.showerror("Input Error","Bad input. Ensure all items are selected")
        return

    root.clipboard_clear()
    root.clipboard_append(val)
def authorLink(event):
    webbrowser.open_new(r"http://keepdream.in")
def homeLink(event):
    webbrowser.open_new(r"https://github.com/geekbozu/NintendoWiSpy")
def aboutWindow():
    window = Toplevel(mainframe)
    window.title("About NintendoWiSpy")
    Label(window,text='NintendoWiSpy').pack(anchor=N)
    Label(window,text='A WiFi enabled Live input viewer for GCN/N64').pack(anchor=N)
    a = Label(window,text="Timothy 'Geekboy1011' Keller",fg="blue", cursor="hand2")
    h = Label(window,text="https://github.com/geekbozu/NintendoWiSpy",fg="blue", cursor="hand2")
    a.bind("<Button-1>", authorLink)
    h.bind("<Button-1>", homeLink)
    a.pack()
    h.pack()
    licframe = LabelFrame(window, text="MIT License:")
    licframe.pack(expand="yes",fill="both")

    scrollbar = Scrollbar(licframe)
    scrollbar.pack(side=RIGHT, fill=Y)
    t = Text(licframe,wrap="word",width=70,height=10,yscrollcommand=scrollbar.set)
    scrollbar.configure(command=t.yview)
    t.pack(side=TOP)
    t.insert(END,"""Copyright 2018 Timothy 'Geekboy1011' Keller

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.""")

def setWifiCreds():
    ser = serial.Serial(serialbox.get())
    ser.baudrate = SerialSpinBoxVar.get()
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    ser.write('WW {} {}\n'.format(SSIDVar.get(),PASSVar.get()))
    ser.close()

def setWinCredMan():
    ser = serial.Serial(serialbox.get())
    ser.baudrate = SerialSpinBoxVar.get()
    window = Toplevel(mainframe)
    window.title("Wifi Manager")

    ser.reset_output_buffer()
    ser.reset_input_buffer()
    ser.write('RW\n')
    SSIDVar.set(ser.readline().strip())
    PASSVar.set(ser.readline().strip())
    ser.close()
    Label(window,text="SSID").pack()
    Entry(window,textvariable=SSIDVar).pack()
    Label(window,text="PASSWORD").pack()
    Entry(window,textvariable=PASSVar).pack()
    Button(window,text="SAVE",command=setWifiCreds).pack()
    window.wait_window(window)

if __name__ == "__main__":
    root = Tk()
    root.title("NintendoWiSpy")
    mainframe = ttk.Frame(root, padding="3 3 12 12")
    mainframe.grid(column=0, row=0, sticky=(N, W, E, S))
    mainframe.columnconfigure(0, weight=1)
    mainframe.rowconfigure(0, weight=1)


    menubar = Menu(root)
    menubar.add_command(label="Wifi Config",command=setWinCredMan)
    menubar.add_command(label="About", command=aboutWindow)
    root.config(menu=menubar)
    SerialSpinBoxVar = IntVar(mainframe)
    SerialSpinBoxVar.set("921600")
    WebSpinBoxVar = IntVar(mainframe)
    WebSpinBoxVar.set("18881")
    HttpSpinBoxVar = IntVar(mainframe)
    HttpSpinBoxVar.set("8888")
    WebSockUrlEntryVar = StringVar(mainframe)
    WebSockUrlEntryVar.set("NintendoSpy.local")
    SSIDVar = StringVar(mainframe)
    PASSVar = StringVar(mainframe)
    Label(mainframe,text='Serial BaudRate').pack(anchor=N)
    Spinbox(mainframe,textvariable=SerialSpinBoxVar).pack(anchor=N)

    Label(mainframe,text='HTTPSERVER port').pack(anchor=N)
    Spinbox(mainframe,from_ = 1, to = 65535,textvariable=HttpSpinBoxVar).pack(anchor=N)

    Label(mainframe,text='WebSocket Host').pack(anchor=N)
    Entry(mainframe,textvariable=WebSockUrlEntryVar).pack(anchor=N)

    Label(mainframe,text='Display Socket Port').pack(anchor=N)
    Spinbox(mainframe,from_ = 1, to = 65535,textvariable=WebSpinBoxVar).pack(anchor=N)

    lists = ttk.Frame(mainframe)
    Label(lists,text='Serial Port').grid(column=0,row=0)
    Label(lists,text='Theme').grid(column=1,row=0)
    serialbox = ttk.Combobox(lists,exportselection=False,height=5)
    serialbox.config(value=[i.device for i in serial.tools.list_ports.comports()])
    #for i in serial.tools.list_ports.comports():
    #    print dir(serialbox.insert)
    #    serialbox.values.insert(END,i.device)
    serialbox.grid(column=0,row=1)
    themebox = ttk.Combobox(lists,exportselection=False,height=5)
    themebox.config(value=[i for i in os.listdir('theme')])
    #for i in os.listdir('theme'):
    #    themebox.values.insert(END,i)

    themebox.grid(column=1,row=1)
    lists.pack()

    HttpButton = Button(mainframe,text='Http Server',command=httpToggle)
    HttpButton.pack(side = LEFT)
    SerialButton = Button(mainframe,text='Serial Forwarder',command=serialToggle)
    SerialButton.pack(side = LEFT)
    Button(mainframe,text='Generate Url',command=genUrl).pack(side = LEFT)
    root.mainloop()


