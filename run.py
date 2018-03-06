import logging
import threading,sys,os
import serial,serial.tools.list_ports
from SimpleHTTPServer import SimpleHTTPRequestHandler
from BaseHTTPServer import HTTPServer
from websocket_server import WebsocketServer
from Tkinter import *
import ttk,tkMessageBox

httpServerRunning = False
serialServerRunning = False

def serialServerLoop():
    global serialServerRunning
    ser = serial.Serial(serialbox.get(serialbox.curselection()))
    ser.baudrate = SerialSpinBoxVar.get()
    ser.write('TS 1\r\n')
    while serialServerRunning:
        server.handle_request()
        line = ser.readline()
        server.handle_request()
        server.send_message_to_all(line)
    ser.write('TS 0\r\n')
    ser.close()
    server.server_close()

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
        httpserver = HTTPServer(('', HttpSpinBoxVar.get()), SimpleHTTPRequestHandler)
        #os.chdir('theme\\' + themebox.get(themebox.curselection()))
        httpthread = threading.Thread(target = httpserver.serve_forever)
        httpthread.daemon = True
        httpthread.start()
        httpServerRunning = True

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
def new_client(client,server):
    print "New client gotten ", client
    
def genUrl():
    if not len(WebSockUrlEntryVar.get()):
        tkMessageBox.showerror("Url Error","WebSocketServer Url can not be blank!")
        return
    try:
        val = "http://localhost:{}/?theme={}&websockport={}&websocketserver={}".format(HttpSpinBoxVar.get(),themebox.get(themebox.curselection()),WebSpinBoxVar.get(),WebSockUrlEntryVar.get())
    except TclError:
        tkMessageBox.showerror("Input Error","Bad input. Ensure all items are selected")
        return
        
    root.clipboard_clear()
    root.clipboard_append(val)
def aboutWindow():
    window = Tk()
    window.title("About NintendoWiSpy")
    Label(window,text='NintendoWiSpy').pack(anchor=N)
    Label(window,text='A WiFi enabled Live input viewer for GCN/N64').pack(anchor=N)
    Label(window,text="Author: Timothy 'Geekboy1011' Keller").pack()
    Label(window,text="HomePage: https://github.com/geekbozu/NintendoWiSpy").pack()
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

        
    
    
if __name__ == "__main__":        
    root = Tk()
    root.title("Web Controller viewer")
    mainframe = ttk.Frame(root, padding="3 3 12 12")
    mainframe.grid(column=0, row=0, sticky=(N, W, E, S))
    mainframe.columnconfigure(0, weight=1)
    mainframe.rowconfigure(0, weight=1)	
    
    
    menubar = Menu(root)
    menubar.add_command(label="Wifi Config")
    menubar.add_command(label="About", command=aboutWindow)
    root.config(menu=menubar)
    SerialSpinBoxVar = IntVar(mainframe)
    SerialSpinBoxVar.set("115200")
    WebSpinBoxVar = IntVar(mainframe)
    WebSpinBoxVar.set("13254")
    HttpSpinBoxVar = IntVar(mainframe)
    HttpSpinBoxVar.set("8888")
    WebSockUrlEntryVar = StringVar(mainframe)
    WebSockUrlEntryVar.set("localhost")
    
    Label(mainframe,text='Serial BaudRate').pack(anchor=N)
    Spinbox(mainframe,textvariable=SerialSpinBoxVar).pack(anchor=N)
    
    Label(mainframe,text='HTTPSERVER port').pack(anchor=N)
    Spinbox(mainframe,from_ = 1, to = 65535,textvariable=HttpSpinBoxVar).pack(anchor=N)
    
    Label(mainframe,text='WebSocket Url').pack(anchor=N)
    Entry(mainframe,textvariable=WebSockUrlEntryVar).pack(anchor=N)
    
    Label(mainframe,text='WebSocket Port').pack(anchor=N)
    Spinbox(mainframe,from_ = 1, to = 65535,textvariable=WebSpinBoxVar).pack(anchor=N)
    
    lists = ttk.Frame(mainframe)
    Label(lists,text='Serial Port').grid(column=0,row=0)
    Label(lists,text='Theme').grid(column=1,row=0)
    serialbox = Listbox(lists,exportselection=False)
    for i in serial.tools.list_ports.comports():
        serialbox.insert(END,i.device)
    serialbox.grid(column=0,row=1)
    themebox = Listbox(lists,exportselection=False)
    for i in os.listdir('theme'):
        themebox.insert(END,i)
    themebox.grid(column=1,row=1)
    lists.pack()
    
    HttpButton = Button(mainframe,text='Http Server',command=httpToggle)
    HttpButton.pack(side = LEFT)
    SerialButton = Button(mainframe,text='Serial Server',command=serialToggle)
    SerialButton.pack(side = LEFT)
    Button(mainframe,text='Generate Url',command=genUrl).pack(side = LEFT)
    root.mainloop()
    
    
