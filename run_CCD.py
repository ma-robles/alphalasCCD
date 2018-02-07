#!/usr/bin/env python3.5

import os
import subprocess
import time


"""
Make script executable with:

$ chmod +x run_CCD.py

and run with

$ ./run_CCD.py
"""

i_time = 10
scans = 1
mode = 0
trigout = 0
d_corr = 0

def executeCCD(i_time,scans,mode, trigout,d_corr):
    """
    Execute command
    """

    # Rename output .csv file with date+time of creation.
    t = time.strftime("%Y-%m-%d_%H%M%S")
    filename = "dataraw/CCD_"
    filename += t
    filename += '_'+str(i_time)
    filename += '_'+str(scans)
    filename += '_'+str(mode)
    filename += '_'+str(trigout)
    filename += '_'+str(d_corr)
    os.system("sudo rmmod ftdi_sio")
    os.system("sudo rmmod usbserial")
    os.system("./CCD_alphalas_2 "+ str(i_time)+" "+str(scans)+ " "+str(mode)+ " "+str(trigout)+ " "+ str(d_corr)+" "+filename)
    #os.rename("CCD-S3600-D_data.csv", "CCD-S3600-D_data_" + t + ".csv")
    print("File stored ",filename)

def create_header(ifile, ofile):
    global i_time
    global scans
    with open(ofile,'w') as opfile:
        with open(ifile,'r') as ipfile:
            for line in ipfile:
                print(line,end='', file=opfile)
        print("Integration time =",i_time,file=opfile)
        print("scans =", scans, file=opfile)

def get_datetime(filename):
    print(filename.split('_'))

while True:
    """
    Never-ending loop.
    """

    # Call CCD executable.
    executeCCD(i_time,scans,mode,trigout,d_corr)

    # Wait this number of minutes before calling the code again.
    N_min = 2
    create_header('dataraw/header.txt','data/out.csv')
    #os.('dataraw/')
    time.sleep(60. * N_min)
