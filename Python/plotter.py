# Plot temperature output from sensor serial
# Code derived from http://www.toptechboy.com/tutorial/python-with-arduino-lesson-11-plotting-and-graphing-live-data-from-arduino-with-matplotlib/

import serial
import numpy
import matplotlib.pyplot as plt
from drawnow import *
import datetime

tempC_meas = []
tempC_cal = []
ramp_times = []

arduinoData = serial.Serial('/dev/tty.usbserial-A6041C4L', 115200) #Creating our serial object named arduinoData
plt.ion() # interactive mode
cnt=0

today = datetime.datetime.today()
timeformat = "%b%d_%H-%M-%S"
filename = "datalog_" + today.strftime(timeformat) + ".txt"

with open(filename, "w") as myfile:
    myfile.write("\n\nRUN at " + str(datetime.datetime.now()) + "\n")
    myfile.write("<t_down, meas_tempC, cal_tempC>\n")

def makeFig(): #Create a function that makes our desired plot
    plt.ylim(10,120)                                 #Set y min and max values
    plt.title('Temperature Data')      #Plot the title
    plt.grid(True)                                  #Turn the grid on
    plt.ylabel('Temp C')                            #Set ylabels
    plt.plot(tempC_meas, 'r-', linewidth=1.5, label='Degrees C Meas.')       #plot the temperature
    plt.legend(loc='upper left')                    #plot the legend
    plt2=plt.twinx()                                #Create a second y axis
    plt.ylim(10,120)                           #Set limits of second y axis- adjust to readings you are getting
    plt2.plot(tempC_cal, 'k--', linewidth=1.5, label='Degrees C Cal') #plot tempC_cal data
    plt2.set_ylabel('Temp C')                    #label second y axis
    plt2.ticklabel_format(useOffset=False)           #Force matplotlib to NOT autoscale y axis
    plt2.legend(loc='upper right')                  #plot the legend
 
while True: # While loop that loops forever
    while (arduinoData.inWaiting()==0): #Wait here until there is data
        pass #do nothing
    arduinoString = arduinoData.readline() #read the line of text from the serial port
    try:
        dataArray = arduinoString.split(',')   #Split it into an array called dataArray
        ramp_time = float( dataArray[0])
        tempC_meas_point = float( dataArray[1])            #Convert first element to floating number and put in temp
        tempC_cal_point =    float( dataArray[2])            #Convert second element to floating number and put in P
        tempC_meas.append(tempC_meas_point)                     #Build our tempC_meas array by appending temp readings
        tempC_cal.append(tempC_cal_point)                     #Building our tempC_cal array by appending P readings
        ramp_times.append(ramp_time)
        cnt=cnt+1
    except:
        pass
    with open(filename, "a") as myfile:
        myfile.write(arduinoString)
    drawnow(makeFig)                       #Call drawnow to update our live graph
    plt.pause(.000001)                     #Pause Briefly. Important to keep drawnow from crashing
    if(cnt>150):                            #If you have 50 or more points, delete the first one from the array
        tempC_meas.pop(0)                       #This allows us to just see the last 50 data points
        tempC_cal.pop(0)