import numpy as np
import csv
import time
import matplotlib.pyplot as plt
import sys
import pickle
import socket
import select
import struct

from gdmif import GDMIf
from gdm442 import GDM442

count_max = 100
count = 0
first_data = True

labels = [113,114,115,116,117,118,119,120,122,123]
m = np.array([[0,0,0,0,0,0,0,0,0,0]]*count_max)

timestart = time.time()


running = True


def handle_close(evt):
    global iface, running
    print('Closed Figure!')
    running = False

    iface.stop()

def step_down():
    global iface
    #unlock
    iface.pwm(1, 2000)
    time.sleep(0.3)

    iface.pwm(2, 2000)
    time.sleep(0.3)

    #lock
    iface.pwm(1, 4000)
    time.sleep(0.3)

    iface.pwm(2, 4000)
    time.sleep(0.3)

iface = GDMIf("/dev/ttyACM0")
iface.start()
iface.set_timing(5000,5,10)
gdm = GDM442(iface, verbose=False)


plt.ion()
fig = plt.figure()
fig.canvas.mpl_connect('close_event', handle_close)

subplot1 = plt.subplot(2, 2, 1)
plt.title("angle")
plt_ang_ha_sin = plt.plot(range(0, len(m)), m[:, 2], color='r', label="ha-sin")
plt_ang_ha_cos = plt.plot(range(0, len(m)), m[:, 3], color='g', label="ha-cos")
plt_ang_va_sin = plt.plot(range(0, len(m)), m[:, 4], color='m', label="va-sin")
plt_ang_va_cos = plt.plot(range(0, len(m)), m[:, 5], color='b', label="va-cos")
plt.autoscale(True)
#plt.axis('equal')
plt.grid()

subplot2 = plt.subplot(2, 2, 2)
plt.title("comp")
plt_comp_x = plt.plot(range(0, len(m)), m[:,8], color='r')
plt_comp_y = plt.plot(range(0, len(m)), m[:,9], color='g')
plt.autoscale(True)
plt.grid()

subplot3 = plt.subplot(2, 2, 3)
plt.title("sector")
plt_ang_ha_sect = plt.plot(range(0, len(m)), m[:, 0], color='r', label="ha")
plt_ang_va_sect = plt.plot(range(0, len(m)), m[:, 1], color='g', label="va")
plt.autoscale(True, 'y')
plt.grid()

#plt.tight_layout()
plt.show()
plt.draw()
#fig, ax = plt.subplots()

skip_cnt = 0
avg_cnt = 1

time_last = 0
state = 0
state_wait = avg_cnt
state_skip = skip_cnt
while running:

    if state == 0:
        if time.time() - time_last > 0.5:
            data = []
            try:
                for l in labels:
                    val = gdm.read_label(l)
                    data.append(val)
            except:
                time.sleep(1)
                continue

            data = np.array(data)

            if state_skip > 0:
                state_skip -= 1
                continue

            if first_data == True:
                first_data = False
                m = np.array([data])#*count_max)
            else:
                if len(m) >= count_max:
                    m = np.delete(m, 0, axis=0)

                m = np.insert(m, len(m), data, axis=0)

            print("Dat:", data)

            plt_ang_ha_sin[0].set_xdata(range(0, len(m)))
            plt_ang_ha_sin[0].set_ydata(m[:,2])
            plt_ang_ha_cos[0].set_xdata(range(0, len(m)))
            plt_ang_ha_cos[0].set_ydata(m[:,3])
            plt_ang_va_sin[0].set_xdata(range(0, len(m)))
            plt_ang_va_sin[0].set_ydata(m[:,4])
            plt_ang_va_cos[0].set_xdata(range(0, len(m)))
            plt_ang_va_cos[0].set_ydata(m[:,5])
            subplot1.set_ylim(-18000, +18000)
            subplot1.set_xlim(0, 100)

            plt_comp_x[0].set_xdata(range(0, len(m)))
            plt_comp_x[0].set_ydata(m[:, 8])
            plt_comp_y[0].set_xdata(range(0, len(m)))
            plt_comp_y[0].set_ydata(m[:, 9])
            subplot2.set_ylim(-20000, +20000)
            subplot2.set_xlim(0, 100)

            plt_ang_ha_sect[0].set_xdata(range(0, len(m)))
            plt_ang_ha_sect[0].set_ydata(m[:, 0])
            plt_ang_va_sect[0].set_xdata(range(0, len(m)))
            plt_ang_va_sect[0].set_ydata(m[:, 1])
            subplot3.set_ylim(0, 60)
            subplot3.set_xlim(0, 100)


            time_last = time.time()
            state_wait -= 1
            if state_wait == 0:
                state = 1
    elif state == 1:
        print("step down")
        step_down()

        state = 2
        state_wait = 15

    elif state == 2:
#        time.sleep(0.1)
        if state_wait > 0:
            state_wait -= 1

        if state_wait <= 0:
            print("wait done")
            state = 0
            state_wait = avg_cnt
            state_skip = skip_cnt

    plt.pause(0.1)

iface.stop()
