import logging
import serial
import serial.tools.list_ports
from SimpleHTTPServer import SimpleHTTPRequestHandler
from BaseHTTPServer import HTTPServer
import threading,os
from websocket_server import WebsocketServer
from Tkinter import *
import ttk

server = WebsocketServer(13254, host='127.0.0.1', loglevel=logging.INFO)
server.timeout = 0

httpserver = HTTPServer(('', 8080), SimpleHTTPRequestHandler)
httpServerRunning = False

serialServerRunning = False

def serialServerLoop():
    global serialServerRunning
    ser = serial.Serial(serialbox.get(serialbox.curselection()))
    ser.baudrate = SerialSpinBoxVar.get()
    while serialServerRunning:
        line = ser.readline()
        server.send_message_to_all(line)
        server.handle_request()

def httpToggle():
    global httpServerRunning 
    global httpserver,httpthread
    if httpServerRunning:
        HttpButton.configure(bg='#F00')
        httpserver.shutdown()
        httpServerRunning = False
        os.chdir(os.path.dirname(__file__))  #back to app root 
    else:
        HttpButton.configure(bg='#0F0')
        httpserver = HTTPServer(('', HttpSpinBoxVar.get()), SimpleHTTPRequestHandler)
        os.chdir('theme\\' + themebox.get(themebox.curselection()))
    
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
        server.timeout = 0
        serialServerRunning = True
        serialthread = threading.Thread(target = serialServerLoop)
        serialthread.daemon = True
        serialthread.start()
        SerialButton.configure(bg='#0F0')

        
        


    
root = Tk()
root.title("Web Controller viewer")
mainframe = ttk.Frame(root, padding="3 3 12 12")
mainframe.grid(column=0, row=0, sticky=(N, W, E, S))
mainframe.columnconfigure(0, weight=1)
mainframe.rowconfigure(0, weight=1)	

SerialSpinBoxVar = IntVar(mainframe)
SerialSpinBoxVar.set("115200")
WebSpinBoxVar = IntVar(mainframe)
WebSpinBoxVar.set("13254")
HttpSpinBoxVar = IntVar(mainframe)
HttpSpinBoxVar.set("8888")

Label(mainframe,text='Serial BaudRate').pack(anchor=N)
Spinbox(mainframe,textvariable=SerialSpinBoxVar).pack(anchor=N)

Label(mainframe,text='HTTPSERVER port').pack(anchor=N)
Spinbox(mainframe,from_ = 1, to = 65535,textvariable=HttpSpinBoxVar).pack(anchor=N)

Label(mainframe,text='WebSocket Port').pack(anchor=N)
Spinbox(mainframe,from_ = 1, to = 65535,textvariable=WebSpinBoxVar).pack(anchor=N)

lists = ttk.Frame(mainframe)
Label(lists,text='Serial Port').grid(column=0,row=0)
Label(lists,text='Theme').grid(column=1,row=0)
serialbox = Listbox(lists)
for i in serial.tools.list_ports.comports():
    serialbox.insert(END,i.device)
serialbox.grid(column=0,row=1)
themebox = Listbox(lists)
for i in os.listdir('theme'):
    themebox.insert(END,i)
themebox.grid(column=1,row=1)
lists.pack()
HttpButton = Button(mainframe,text='Http Server',command=httpToggle)
HttpButton.pack(side = LEFT)
SerialButton = Button(mainframe,text='Serial Server',command=serialToggle)
SerialButton.pack(side = LEFT)
Button(mainframe,text='Generate Url').pack(side = LEFT)
root.mainloop()

    
