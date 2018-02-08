#!/usr/bin/env python3.5

import os
import subprocess
import time
import os.path
import numpy as np


"""
Make script executable with:

$ chmod +x run_CCD.py

and run with

$ ./run_CCD.py
"""

i_time = 10
scans = 2
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
    os.system("./CCD_alphalas_2 "+  str(i_time)+" "+str(scans)+ " "+str(mode)+ " "+str(trigout)+ " "+ str(d_corr)+" "+filename+".csv")
    #os.rename("CCD-S3600-D_data.csv", "CCD-S3600-D_data_" + t + ".csv")

    print("File stored... ",filename+".csv")
    return filename+".csv"
    

def create_header(ifile, ofile):
    global i_time
    global scans
    with open(ofile,'w') as opfile:
        with open(ifile,'r') as ipfile:
            for line in ipfile:
                print(line,end='', file=opfile)
        print("Integration time =",i_time,file=opfile)
        print("scans =", scans, file=opfile)

def get_prom(filename):
    data = [np.zeros(3648)]
    print(data)
    with open(filename,'r') as ipfile:
        for line in ipfile:
            datan = line[:-1].split(',')
            data = np.append(data,[datan],axis=0)
    data = data[1:].astype(np.int)
    print('data', data, np.size(data))
    print('prom',np.mean(data,axis=0))
    prom=np.mean(data,axis=0)
    str_prom = ''
    for d in prom:
        str_prom += str(d)+','
    
    #print('prom:', str_prom[:-1])
    return str_prom[:-1]

def create_ofile(ifilename, header):
 
    t = time.strftime("%Y-%m-%d")
    ofilename = "data/CCD-prom-"+t+".csv"
    if os.path.isfile(ofilename):
        newfile = False
    else:
        newfile = True
    #header
    opfile = open(ofilename, 'a')
    if newfile == True:
        with open(header,'r') as ipfile:
            for line in ipfile:
                print(line, end='', file=opfile)
    #promedio
    datetime = ifilename.split('_')
    mydate = datetime[1]
    mytime = datetime[2][0:2]+":"
    mytime += datetime[2][2:4]+":"
    mytime += datetime[2][4:6]
    itime = datetime[3]

    data = get_prom(ifilename)
    #datetime[2][2:2]=':'
    print( mydate,mytime,itime, data, sep=',',file=opfile)
        
    opfile.close()

def get_datetime(filename):
    print(filename.split('_'))

print("Iniciando...")
while True:
    """
    Never-ending loop.
    """

    # Call CCD executable.
    datafile = executeCCD(i_time,scans,mode,trigout,d_corr)
    print('test=',datafile)
    header = "dataraw/header.txt"
    create_ofile(datafile,header)
    # Wait this number of minutes before calling the code again.
    N_min = 1
    #create_header('dataraw/header.txt','data/out.csv')
    #os.('dataraw/')
    time.sleep(60. * N_min)
