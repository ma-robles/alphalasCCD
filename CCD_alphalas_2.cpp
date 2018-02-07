// CCD-S3600-D_D2XX_Example.cpp : Defines the entry point for the console application.
//
/*
	ALPHALAS CCD-S3600-D-(UV) Example Code (www.alphalas.com)
	v1.10 : Added trigger output control (enable/disable) for FW A1.10

	Copyright (c) 2011 ALPHALAS GmbH. ALL RIGHTS RESERVED. Under the copyright laws, this code may NOT be redistributed,
	reproduced or transmitted in any form, electronic or mechanical, including photocopying, recording, storing in an information retrieval system, or translating,
	in whole or in part, without the prior written consent of ALPHALAS GmbH. This software may be used only with the hardware it has been purchased with.
	Disclaimer of Warranty: THIS SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY WARRANTY. THE COPYRIGHT HOLDER PROVIDES THE SOFTWARE AS IS; WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE SOFTWARE IS WITH THE CUSTOMER. SHOULD THE SOFTWARE PROVE DEFECTIVE, THE CUSTOMER ASSUMES THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
	Limitation of Liability: IN NO EVENT WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE SOFTWARE AS PERMITTED ABOVE, BE LIABLE TO THE CUSTOMER FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE SOFTWARE (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY THE CUSTOMER OR THIRD PARTIES OR A FAILURE OF THE SOFTWARE TO OPERATE WITH ANY OTHER SOFTWARE), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES. THIS LIMITATION OF THE LIABILITY WILL APPLY REGARDLESS OF THE FORM OF ACTION, WHETHER IN CONTRACT OR TORT, INCLUDING NEGLIGENCE.

	This C++ example code demonstrates how to use the CCD-S3600-D-(UV) device. It works with Microsoft Visual C++ for Windows as well as GCC C++ for Linux, Mac OS X, etc.
	This example will initialize & configure the device with the user-specified parameters, then it will acquire and fetch the data and will finally print the values on the screen
	and write them into a comma-separated CSV text file as well as a binary file (big-endian).

	MICROSOFT VISUAL C++ FOR WINDOWS INSTRUCTIONS:
	The example code works with Microsoft Visual C++ for Windows (also with the Express edition). The VC++ project has been created with VC++ 2010.
	The VC++ project requires ftd2xx.dll which should have been already installed with the FTDI D2XX driver into the Windows system folder.
	This project also requires ftd2xx.lib and ftd2xx.h which are included with the FTDI D2XX driver available on the FTDI website. Both files must be made available in the same folder as this source code file.
	ftd2xx.lib must be also added to the Visual C++ project through Project > CCD-S3600-D_D2XX_Example Properties... > Configuration Properties > Linker > Input > Additional Dependencies
	Make sure that ftd2xx.lib is added to that additional dependencies list of .lib files separated by a semicolon and for ALL CONFIGURATIONS (Debug and Release).

	GNU COMPILER COLLECTION (GCC) FOR LINUX, MAC OS X, ETC. INSTRUCTIONS:
	The example code also works with GCC (GNU Compiler Collection) for Linux, Mac OS X and similar OS.
	Make sure that the FTDI D2XX Driver for Linux or Mac OS X has been installed from http://www.ftdichip.com/Drivers/D2XX.htm.
	Make sure that the GCC C++ compiler is installed correctly. In order to install GCC for Mac OS X you should install Apple's Xcode development environment.
	Make sure that the ftd2xx.h and WinTypes.h files from the Linux or Mac OS X driver are accessible from this C++ program (e.g. place them in the same folder).
 	To build this example, use the following GCC statement (assuming you have the D2XX library installed in the /usr/local/lib directory).
	g++ -o CCD-S3600-D_D2XX_Example CCD-S3600-D_D2XX_Example.cpp -L. -lftd2xx -Wl,-rpath /usr/local/lib
	Notes for Linux: Because the example uses the D2XX driver you need to first unload any VCP drivers loaded by the kernel by running
	"sudo rmmod ftdi_sio" and "sudo rmmod usbserial". Another important thing is that the FTDI library makes use of libusb. In order to get access to a USB device you have to run all programs that access the USB device as a root user using sudo. For the compiled example code this means starting it like this: sudo ./CCD-S3600-D_D2XX_Example
*/
// includes for Microsoft compilers only
#if defined _MSC_VER
	#include "stdafx.h"
	#include <Windows.h>
#endif
// includes for all compilers
#include <stdio.h>
#include <stdlib.h>
#include "ftd2xx.h"
//Parameters
char filename[30];
char *parp0=filename;


// define the command byte constants for each command
const BYTE	CMD_SET_INTEGRATION_TIME = 0xC1;
const BYTE	CMD_SET_SCANS_PER_ACQUISITION = 0xC2;
const BYTE	CMD_SET_CCD_OPERATING_MODE = 0xC3;
const BYTE	CMD_SET_TRIGOUT_BEFORE_INTEGRATION = 0xC4;
const BYTE	CMD_SET_HW_DARK_CORRECTION = 0xC5;
const BYTE	CMD_INITIATE_ACQUISITION_FOR_NUM_OF_SCANS = 0xC6;
const BYTE	CMD_TRIG_OUT_ENABLE = 0xE1;	// added in FW A1.10

// FTDI device name
char DeviceName[] = "CCD-S3600-D(-UV) B";

// number of active pixels on CCD
const DWORD NUMBER_OF_ACTIVE_CCD_PIXELS = 3648;

// create unions to be able to access the individual bytes of DWORD and WORD variables
union DWORD_union
{
   DWORD asDWORD;	// used for the whole numbers
   BYTE	 asBYTES[sizeof(DWORD)];	// used to access the individual bytes of the number
};

union WORD_union
{
   WORD asWORD;	// used for the whole numbers
   BYTE	 asBYTES[sizeof(DWORD)];	// used to access the individual bytes of the number
};

// global variables
FT_HANDLE ftHandle; // Handle to the FTDI device
FT_STATUS ftStatus; // Result of each FTDI D2XX call
BYTE byOutputBuffer[256]; // Buffer to hold data to be sent to the FT2232H (in bytes)
BYTE byInputBuffer[8192]; // Buffer to hold data read from the FT2232H (in bytes) - MUST BE LARGE ENOUGH TO HOLD DATA FOR ONE CCD FRAME
DWORD dwNumBytesToSend = 0;	// Index to the output buffer
DWORD dwNumBytesSent = 0;	// Count of actual bytes sent - used with FT_Write
DWORD dwNumBytesToRead = 0;	// Number of bytes available to read in the driver's input buffer
DWORD dwNumBytesRead = 0;	// Count of actual bytes read - used with FT_Read
FILE *output_binary_file;	// binary data file for output
FILE *output_text_file;		// CSV text data file for output


// function declarations: all functions that are defined AFTER main() must be declared first
void CCD_Initialize(void);
void CCD_SendCommandAndProcessResponse(BYTE Command, DWORD Data, DWORD SCANS_PERACQUISITION);
void CreateAndOpenOutputFiles(void);
void CloseOutputFiles(void);
void exit_with_error(void);
void close_device_and_exit_with_error(void);


// the main() function
int main(int argc, char *argv[])
{
        //Integration time
	int int_time=atoi(argv[1]);
        //scans per acquisition
        int scans = atoi(argv[2]);
        //mode
        int ccd_mode = atoi(argv[3]);
        //trigout
        int trigout = atoi(argv[4]);
        //HW dark correction
        int dark_c = atoi(argv[5]);
        //filename
        parp0 = argv[6];

	printf("parms = %i, %i, %i, %i, %i, %s\n", int_time,scans,ccd_mode, trigout, dark_c, parp0);
        
// Define the values for each CCD parameter
const DWORD INTEGRATION_TIME = int_time; // 5000 �s integration time
const DWORD SCANS_PER_ACQUISITION = scans;	// number of scans (frames) per acquisition
const DWORD CCD_OPERATING_MODE = ccd_mode;		// default operating mode = internally synchronized, continuously running with software capture start
const DWORD TRIGOUT_BEFORE_INTEGRATION = trigout;	// no trigout offset before integration
const DWORD HW_DARK_CORRECTION = dark_c;	// no hardware dark correction ( 0 = off, 1 = on)

	printf("-----------------------------------\n");
	printf(" ALPHALAS CCD-S3600-D-(UV) Example \n");
	printf(" Copyright (C) 2012 by ALPHALAS GmbH \n");
	printf(" www.alphalas.com \n");
	printf("-----------------------------------\n");
	printf("This example demonstrates how to use the CCD-S3600-D-(UV) device.\n");
	printf("It will initialize & configure the device with the user-specified parameters,\n");
	printf("then it will acquire and fetch the data and will finally print the values on\n");
	printf("the screen and write them into a comma-separated CSV text file\n");
	printf("as well as a binary file (big-endian).\n\n");

	printf("Press ENTER to begin...\n");
	// getchar();

	// Initialize the CCD
	printf("Beginning device initialization:\n");
	CCD_Initialize();
	// Disable trigger output to prevent glitches during configuration phase (works with FW A1.10 or later!)
	printf("Disabling trigger output during configuration... ");
	CCD_SendCommandAndProcessResponse(CMD_TRIG_OUT_ENABLE, 0,SCANS_PER_ACQUISITION);	// disable trigger output
	// Setup CCD parameters
	printf("Setting CCD operating mode %d... ", CCD_OPERATING_MODE);
	CCD_SendCommandAndProcessResponse(CMD_SET_CCD_OPERATING_MODE, CCD_OPERATING_MODE, SCANS_PER_ACQUISITION);	// set the operating mode of the CCD
	printf("Setting number of scans per acqusition to %d... ", SCANS_PER_ACQUISITION);
	CCD_SendCommandAndProcessResponse(CMD_SET_SCANS_PER_ACQUISITION, SCANS_PER_ACQUISITION, SCANS_PER_ACQUISITION);	// set the scans per acquisition
	printf("Setting trig out before integration start offset to %d us... ", TRIGOUT_BEFORE_INTEGRATION);
	CCD_SendCommandAndProcessResponse(CMD_SET_TRIGOUT_BEFORE_INTEGRATION, TRIGOUT_BEFORE_INTEGRATION, SCANS_PER_ACQUISITION);	// set trig out before integration start offset
	if (HW_DARK_CORRECTION == 1)
		printf("Turning ON hardware dark correction... ");
	else
		printf("Turning OFF hardware dark correction... ");
	CCD_SendCommandAndProcessResponse(CMD_SET_HW_DARK_CORRECTION, HW_DARK_CORRECTION,SCANS_PER_ACQUISITION);	// set hardware dark correction
	printf("Setting integration time to %d us... ", INTEGRATION_TIME);
	CCD_SendCommandAndProcessResponse(CMD_SET_INTEGRATION_TIME, INTEGRATION_TIME, SCANS_PER_ACQUISITION);	// set the integration time
	// Enable trigger output after having done the configuration (works with FW A1.10 or later!)
	printf("Enabling trigger output after configuration... ");
	CCD_SendCommandAndProcessResponse(CMD_TRIG_OUT_ENABLE, 1,SCANS_PER_ACQUISITION);	// enable trigger output

	CreateAndOpenOutputFiles();	// create the files where we are going to output the data from the CCD and open them for writing

	// Acquire preset number of scans, fetch data and display + store it to files
	printf("Initiating acquisition for %d scan(s), waiting for it to finish and fetching and displaying the data...\n", SCANS_PER_ACQUISITION);
	CCD_SendCommandAndProcessResponse(CMD_INITIATE_ACQUISITION_FOR_NUM_OF_SCANS, 0, SCANS_PER_ACQUISITION);	// initiate acquisition

	// Disable trigger output after having fetched the data (works with FW A1.10 or later!)
	printf("Disabling trigger output after having fetched all data... ");
	CCD_SendCommandAndProcessResponse(CMD_TRIG_OUT_ENABLE, 0, SCANS_PER_ACQUISITION);	// disable trigger output

	CloseOutputFiles(); // close the output files

	// Close the device
	printf("Closing device...\n");
	FT_Close(ftHandle);	// Close the port
	printf("-----\n");
	printf("Program finished successfully. Press ENTER to exit the program...\n");
	//getchar();

	return 0;
}	// main()

///////////////////////////////////////////////////////////////////////////////////////////
// Sends a command byte + optional data bytes to the CCD and gets & processes the response
///////////////////////////////////////////////////////////////////////////////////////////
void CCD_SendCommandAndProcessResponse(BYTE Command, DWORD Data, DWORD SCANS_PER_ACQUISITION)
{
	//****** SEND CMD + DATA ********
	dwNumBytesToSend = 0;	// reset
	// Add the command byte to the output buffer
	byOutputBuffer[dwNumBytesToSend++] = Command;

	// Present data as union to be able to access the individual bytes
	DWORD_union DataAsUnion;
	DataAsUnion.asDWORD = Data;

	// Determine how many data bytes have to be sent with the selected command
	DWORD DataBytesForCommand;
	switch (Command)
	{
		// commands that require 32 bit = 4 bytes of data
		case CMD_SET_INTEGRATION_TIME:
		case CMD_SET_SCANS_PER_ACQUISITION:
		case CMD_SET_TRIGOUT_BEFORE_INTEGRATION:
			DataBytesForCommand = 4;
			break;
		// commands that require 8 bit = 1 byte of data
		case CMD_SET_CCD_OPERATING_MODE:
		case CMD_SET_HW_DARK_CORRECTION:
		case CMD_TRIG_OUT_ENABLE:
			DataBytesForCommand = 1;
			break;
		// commands that don't require any data
		case CMD_INITIATE_ACQUISITION_FOR_NUM_OF_SCANS:
			DataBytesForCommand = 0;
			break;
        default:
			DataBytesForCommand = 0;
			break;
    }

	// Copy the data that will be sent with the command to the output buffer
	// the following code requires that the host computer stores DWORDs as little-endian in memory - if your computer stores DWORDs in big-endian, the loop below needs to be modified!
	for (DWORD i = (sizeof(DWORD)-DataBytesForCommand); i < sizeof(DWORD); i++)
	{
		// printf("0x%x\n", DataAsUnion.asBYTES[sizeof(DWORD)-1-i]);
		byOutputBuffer[dwNumBytesToSend++] = DataAsUnion.asBYTES[sizeof(DWORD)-1-i];
	}

	// Finally send command + data to the device
	ftStatus = FT_Write(ftHandle, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);	// Send command + data
	if (ftStatus != FT_OK)
	{
		printf("Send command failed with error %d.\n", ftStatus);
		close_device_and_exit_with_error();
	}
	if (dwNumBytesSent != dwNumBytesToSend)
	{
		printf("Unexpected number of bytes sent to the device.\n");
		close_device_and_exit_with_error();
	}

	//********* GET AND PROCESS RESPONSE ***************
	// Depending on the sent command, determine how many response bytes are expected and how many fetch iterations we have to do (important if the user has set >1 scans)
	DWORD ExpectedResponseBytes;
	DWORD FetchIterations;
	switch (Command)	// depending on the command
	{
		case CMD_INITIATE_ACQUISITION_FOR_NUM_OF_SCANS:	// if we have the acquisition command
			ExpectedResponseBytes = NUMBER_OF_ACTIVE_CCD_PIXELS * 2;	// we expect to receive 2 bytes for every pixel in the scan
			FetchIterations = SCANS_PER_ACQUISITION;					// if we have set multiple scans per acquisition, we need to fetch scans more than once
			break;
		default:
			ExpectedResponseBytes = 1;	// for any other command we expect only 1 byte (ACK or NACK) as response
			FetchIterations = 1;			// we have to fetch data only once
			break;
	}

	// do ... while loop for each iteration
	do {

		//printf("Waiting for %d response bytes from the device...\n", ExpectedResponseBytes);
		// Check incoming queue status until the expected response bytes for this iteration have been received
		do
		{
			ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);		// Get the number of bytes waiting in the receive queue
			if (ftStatus != FT_OK)
			{
				printf("GetQueueStatus failed with error %d\n", ftStatus);
				close_device_and_exit_with_error();
			}
		} while (dwNumBytesToRead < ExpectedResponseBytes);	// while expected number of bytes not arrived yet
		// Now fetch the bytes from one scan
		ftStatus = FT_Read(ftHandle, byInputBuffer, ExpectedResponseBytes, &dwNumBytesRead); 	// Read out the data from input queue
		if (ftStatus != FT_OK)
		{
			printf("Reading the bytes failed with error %d\n", ftStatus);
			close_device_and_exit_with_error();
		}
		if (dwNumBytesRead != ExpectedResponseBytes)	// IMPORTANT: ftStatus may be ok, but due to a timeout the number of read bytes may differ, always check! See d2xx guide
		{
			printf("Timeout! Read wrong number of bytes.\n");
			close_device_and_exit_with_error();
		}

		// Now process the response depending on the command
		if (Command == CMD_INITIATE_ACQUISITION_FOR_NUM_OF_SCANS)	// if we have the acquisition command
		{
			// AT THIS POINT THE FETCHED PIXEL VALUES FROM ONE SCAN SHOULD BE AVAILABLE IN THE byInputBuffer BYTE ARRAY
			printf("About to display the values from scan %d/%d and write them to the CSV text file. Press enter to proceed...\n", (SCANS_PER_ACQUISITION-FetchIterations+1), SCANS_PER_ACQUISITION);
			//getchar();
			printf("SCAN %d/%d: Displaying the fetched values for all %d pixels and writing them to the CSV text file: \n", (SCANS_PER_ACQUISITION-FetchIterations+1), SCANS_PER_ACQUISITION, NUMBER_OF_ACTIVE_CCD_PIXELS);
			WORD_union PixelValue;
			for (DWORD i = 0; i < NUMBER_OF_ACTIVE_CCD_PIXELS; i++)
			{
				// Load values in little-endian byte order instead of the original big-endian byte order
				PixelValue.asBYTES[0] = byInputBuffer[i*2 +1];
				PixelValue.asBYTES[1] = byInputBuffer[i*2];
				printf("%d ", PixelValue.asWORD);	// display the value on screen
				fprintf(output_text_file, "%d", PixelValue.asWORD);	// also write the value to CSV text file
				if (i == NUMBER_OF_ACTIVE_CCD_PIXELS-1)
					fprintf(output_text_file, "\n");	// write new line as complete scan delimiter to file
				else
					fprintf(output_text_file, ",");		// write comma as value delimiter to file
			}
			printf("\n");

			if (!output_text_file)	// if error in text file
			{
			  printf("ERROR: Could not write to text output file!\n");
			  close_device_and_exit_with_error();	// must also close device!
			  fclose(output_text_file);
			}

			// Write the complete scan into the binary file, keep big-endian format
			printf("Writing data from current scan to binary output file... ");
			fwrite(byInputBuffer, 1, NUMBER_OF_ACTIVE_CCD_PIXELS*2, output_binary_file);
			if (!output_binary_file)	// if error in binary file
			{
			  printf("ERROR: Could not write to binary output file!\n");
			  close_device_and_exit_with_error();	// must also close device!
			  fclose(output_binary_file);
			}
			else
			{
				printf("OK\n");
			}
		}
		else	// if we have another CCD COMMAND
		{
			// check if device replied with ACK, NACK or something unexpected
			if (byInputBuffer[0] == 0x06)	// response was ACK
			{
				printf("OK\n", byInputBuffer[0]);
			}
			else if (byInputBuffer[0] == 0x15)	// response was NACK
			{
				printf("The command has NOT been executed succesfully.\n\n", byInputBuffer[0]);
				close_device_and_exit_with_error();
			}
			else	// response was something unexpected
			{
				printf("Error: The device returned an unexpected response.\n\n", byInputBuffer[0]);
				close_device_and_exit_with_error();
			}
		}

		FetchIterations--; // decrease number of iterations to do (this is used to be able to fetch multiple scans)

	} while (FetchIterations != 0);	// finished?

}	// end of CCD_SendCommandAndProcessResponse()


// CREATES BINARY FILE AND TEXT (CSV) FILE TO STORE DATA VALUES
void CreateAndOpenOutputFiles(void)
{
	printf("Creating CCD-S3600-D_data.bin file... ");
	// Create the file. If the file already exists, the old version is deleted.
	output_binary_file = fopen("CCD-S3600-D_data.bin", "wb");	// create + open for writing in binary mode (if the file already exists, the old version is overwritten)
	if (!output_binary_file) // check if file open error
	{
      printf("ERROR: Could not create output file!\n");
	  close_device_and_exit_with_error();	// must also close device!
	}
	else
	{
		printf("OK\n");
	}

	printf("Creating file: %s ",parp0);
	output_text_file = fopen(parp0, "w");	// create + open for writing in text mode (if the file already exists, the old version is overwritten)
	if (!output_text_file) // check if file open error
	{
      printf("ERROR: Could not create output file!\n");
	  close_device_and_exit_with_error();	// must also close device!
	}
	else
	{
		printf("OK\n");
	}
}

// CLOSES THE FILES
void CloseOutputFiles(void)
{
	printf("Closing both output files...\n");
	fclose(output_binary_file);
	fclose(output_text_file);
}

// exit with error: called whenever something went wrong and we need to exit the program - does NOT close the device before exiting
void exit_with_error(void)
{
	printf("-----\n");
	printf("An error occurred. Press ENTER to exit the program...\n");
	//getchar();
	exit(1);
}

// close device and exit with error: called whenever something went wrong and we need to exit the program - FIRST CLOSES the device before exiting
void close_device_and_exit_with_error(void)
{
	printf("Closing device...\n");
	FT_Close(ftHandle);						// Close the port
	exit_with_error();
}


// function to initialize the device
void CCD_Initialize(void)
{
	// OPEN DEVICE
	printf("Opening device...\n");
	ftStatus = FT_OpenEx(DeviceName, FT_OPEN_BY_DESCRIPTION, &ftHandle);	// Open port B of the CCD-S3600-D(-UV) device and return its handle for subsequent accesses
	if (ftStatus != FT_OK) // Did the command execute OK?
	{
		printf("Error: Could not open device.\n");
		exit_with_error();
	}
	// SETUP DEVICE & DRIVER FOR HIGH-SPEED OPERATION --- this is a standard procedure that can be kept as is
	printf("Setting up device & driver...\n");
	// Reset USB device
	ftStatus = FT_ResetDevice(ftHandle);
	if (ftStatus != FT_OK)
	{
		printf("ResetDevice failed with error %d\n", ftStatus);
		close_device_and_exit_with_error();
	}
	// Purge USB receive buffer first by reading out all old data from FT2232H receive buffer
	ftStatus = FT_GetQueueStatus(ftHandle, &dwNumBytesToRead);  // Get the number of bytes in the FT2232H receive buffer
	if (ftStatus != FT_OK)
	{
		printf("GetQueueStatus failed with error %d\n", ftStatus);
		close_device_and_exit_with_error();
	}
	if ((ftStatus == FT_OK) && (dwNumBytesToRead > 0))
		ftStatus = FT_Read(ftHandle, byInputBuffer, dwNumBytesToRead, &dwNumBytesRead);  	//Read out the data from FT2232H receive buffer
	if (ftStatus != FT_OK)
	{
		printf("Read failed with error %d\n", ftStatus);
		close_device_and_exit_with_error();
	}
	// Purge RX and TX Buffers
	ftStatus = FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
	if (ftStatus != FT_OK)
	{
		printf("Purge failed with error %d\n", ftStatus);
		close_device_and_exit_with_error();
	}
	// Set USB request transfer sizes to 64K (=max)
	ftStatus = FT_SetUSBParameters(ftHandle, 65536, 65535);
	if (ftStatus != FT_OK)
	{
		printf("SetUSBParameters failed with error %d\n", ftStatus);
		close_device_and_exit_with_error();
	}
	// Disable event and error characters
	ftStatus = FT_SetChars(ftHandle, false, 0, false, 0);
	if (ftStatus != FT_OK)
	{
		printf("SetChars failed with error %d\n", ftStatus);
		close_device_and_exit_with_error();
	}
	// Sets the read and write timeouts in milliseconds
	ftStatus = FT_SetTimeouts(ftHandle, 0, 0); // disable timeouts to allow longer integration times
	if (ftStatus != FT_OK)
	{
		printf("SetTimeouts failed with error %d\n", ftStatus);
		close_device_and_exit_with_error();
	}
	// Set the latency timer to 2mS (default is 16mS)
	ftStatus = FT_SetLatencyTimer(ftHandle, 2); // 2 ms seems to be the min. value according to the d2xx guide
	if (ftStatus != FT_OK)
	{
		printf("SetLatencyTimer failed with error %d\n", ftStatus);
		close_device_and_exit_with_error();
	}
	// Turn on flow control to synchronize IN requests
	ftStatus = FT_SetFlowControl(ftHandle, FT_FLOW_RTS_CTS, 0x00, 0x00);
	if (ftStatus != FT_OK)
	{
		printf("SetFlowControl failed with error %d\n", ftStatus);
		close_device_and_exit_with_error();
	}
	// Reset controller
	ftStatus = FT_SetBitMode(ftHandle, 0x0, 0x00);
	if (ftStatus != FT_OK)
	{
		printf("SetBitMode Reset failed with error %d\n", ftStatus);
		close_device_and_exit_with_error();
	}
	// Initialization finished
	printf("Device initialized successfully.\n\n");

}
