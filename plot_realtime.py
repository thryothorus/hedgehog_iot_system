


import sys
from math import sin
from threading import Thread
from time import sleep
import matplotlib.cm as cm
from PyQt6.QtWidgets import QApplication
import pyqtgraph as pg
from pglive.sources.data_connector import DataConnector
from pglive.sources.live_plot import LiveLinePlot
from pglive.sources.live_plot_widget import LivePlotWidget



import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import numpy as np
import time
import serial
ser = serial.Serial('COM11', 115200)

app = QApplication(sys.argv)
running = True



grp_key = dict()
start = time.time()
while True:
        group, key, value = ser.readline().strip().decode().split(":")
        if group not in grp_key:
            grp_key[group] = dict()

        if key not in grp_key[group]:
            grp_key[group][key] = 0;    

        grp_key[group][key]+=1
        if (time.time() - start) > 2:
            break


group_widget_map = dict();
for grp in grp_key:
    group_widget_map[grp] = LivePlotWidget(title="Group: "+grp);
    
num_keys = len([[x,y]  for x in grp_key for y in grp_key[x]]);
rgb_colors = [cm.Set2(i)[:3] for i in range(num_keys)]



key_plot_map = dict()
key_data_map = dict()
idx = 0
for grp, key in [[x,y]  for x in grp_key for y in grp_key[x]]:
    c_colr = [int(255*x) for x in rgb_colors[idx % 8]]
    plot_curve = LiveLinePlot(pen=pg.mkPen(color=c_colr), name=key)
    key_plot_map[key] = plot_curve
    group_widget_map[grp].addItem(plot_curve)
    data_connector = DataConnector(plot_curve, max_points=100+ (5*grp_key[grp][key]), update_rate=1000)
    key_data_map[key] = data_connector
    idx+=1








def data_generator(key_data_map):
    while True:
        try:
            group, key, value = ser.readline().strip().decode().split(":")            
            data_connector = key_data_map[key]
            data_connector.cb_append_data_point(float(value), time.time())
        except Exception as e:
            print(e)
            print(group, key, value)
        
        

for _, widget in group_widget_map.items():
    widget.show();
t = Thread(target=data_generator, args=(key_data_map,))
t.daemon = True
t.start()
app.exec()
running = False