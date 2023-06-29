// PhotonOS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#pragma comment (lib, "legacy_stdio_definitions.lib")


#include <stdio.h>
//#include "CyAPI.h"


#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <conio.h>
#include <time.h>
#include <bitset>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include "CyAPI.h"

//NOTE: CyAPI.h requires linking to Setupapi.lib
#define REALTIME_FUNCTION correlate
#define REALTIME_FUNCTION_2 correlateDelayed

//THIS IS THE *ORIGINAL* CORRELATE FUNCTION FROM THE NIST CODE; NO DELAY BUFFER
void correlate(unsigned char* data, int length, __int64* stats)
{
	int i;
	int dlength = length % 4;

	for (i = 0; i < length; i += 4)
	{
		int state[4];
		state[0] = (data[i + 3] & 0x08) ? true : false;
		state[1] = (data[i + 3] & 0x10) ? true : false;
		state[2] = (data[i + 3] & 0x20) ? true : false;
		state[3] = (data[i + 3] & 0x40) ? true : false;

		//# of starts
		stats[0] += data[i + 3] & 0x80 ? 1 : 0;

		stats[3] += (int)state[2];//state 2
		stats[4] += (int)state[3];//state 3

		if (state[0])
		{
			stats[1] += 1; //state 0;

			stats[5] += (state[1]) ? 1 : 0; //states 0&1
			stats[6] += (state[2]) ? 1 : 0; //states 0&2
			stats[7] += (state[3]) ? 1 : 0; //states 0&3

			stats[11] += (state[1] & state[2]) ? 1 : 0; //states 0,1&2
			stats[12] += (state[1] & state[3]) ? 1 : 0; //states 0,1&3
			stats[13] += (state[2] & state[3]) ? 1 : 0; //states 0,2&3

			stats[15] += (state[1] & state[2] & state[3]) ? 1 : 0; //states 0,1,2&3
		}

		if (state[1])
		{
			stats[2] += 1;//state 1

			stats[8] += (state[2]) ? 1 : 0; //state 1&2
			stats[9] += (state[3]) ? 1 : 0; //state 1&3

			stats[14] += (state[2] & state[3]) ? 1 : 0;  //state 1&2&3              
		}
		stats[10] += (state[2] & state[3]) ? 1 : 0; //state 2&3
	}
	return;
}
// END ORIGINAL CORRELATE FUNCTION

//THIS IS A FUNCTION THAT *SHOULD* TAKE TWO VECTORS, AND CHECK WHAT THE INDICES OF ELEMENTS WHERE THEY ARE EQUAL ARE; IE (1,2,3) AND (4, 1, 5) WOULD RETURN (0, 1). APPEARS TO BE A BIT BROKEN WHEN THE DELAY IS TURNED OUT
std::pair<std::vector<int>, std::vector<int>> findEqualElements(const std::vector<int>& vec1, const std::vector<int>& vec2) {
    std::unordered_map<int, std::vector<int>> map;
    int size = vec1.size();
  
    for (int i = 0; i < size; ++i) {
        map[vec1[i]].push_back(i);  // Store the indices of each element from vec1
    }
  
    std::vector<int> indicesVec1;
    std::vector<int> indicesVec2;
  
    for (int i = 0; i < size; ++i) {
        if (map.count(vec2[i]) > 0) {
            const std::vector<int>& indices = map[vec2[i]];
            for (int index : indices) {
                indicesVec1.push_back(index);
                indicesVec2.push_back(i);
            }
        }
    }

    return std::make_pair(indicesVec1, indicesVec2);
}
//END INDEX MATCHING FUNCTION

//THIS IS A FUNCTION TO TAKE AN UNSINGED CHAR AND FLIP IT; VERY USEFUL FOR CHANGING THE "ENDIENESS" OF OUR DATA, AS CAN BE SEEN BELOW
unsigned char reverse(unsigned char b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}
//END FLIPPING FUNCTION

//FUNCTION TO REVERSE AN ENTIRE 32 BIT NUMBER; ALSO USEFUL FOR CHANGING ENDIENESS.
int N = 32;
template<std::size_t N>
void reverseBit(std::bitset<N>& b) {
	for (std::size_t i = 0; i < N / 2; ++i) {
		bool t = b[i];
		b[i] = b[N - i - 1];
		b[N - i - 1] = t;
	}

}
//END FLIPPING FUNCTION


//REMOVE DUPLICATES FROM A VECTOR; JUST IN CASE A TIMESTAMP IS REPEATED, WE CAN ERASE IT. SHOULDN'T HAPPEN, BUT I WROTE THIS JUST IN CASE.
void removeDupes(std::vector<int>& v)
{
	auto end = v.end();
	for (auto it = v.begin(); it != end; ++it) {
		end = std::remove(it + 1, end, *it);
	}

	v.erase(end, v.end());
}
//END REMOVE DUPLICATE FUNCTION

//NEW CORRELATION FUNCTION
void correlateDelayed(std::vector<int> newTimes, std::vector<std::bitset<5>> clicks, __int64* statsDelay, int delay, std::vector<int> OldLateTimes, std::vector<std::bitset<5>> OldLateClicks)
{
	//IMPORTANT: USES A DELAY BUFFER CALLED OldLateTimes and OldLateClicks to store delayed timestamp data that cannot have a counterpart in the undelayed data (as it hasn't been pulled off the board yet)

	std::vector<int> delayTimes = OldLateTimes; //Makes a new vector that adds the current delay buffer to the front of it
	std::vector<int> earlyTimes = {}; //Makes a new vector composed of the times from the last pull from FPGA
	std::vector<int> timeDelay = {}; //Makes a copy of the above


	for (int i = 0; i < newTimes.size(); i += 1) {
		timeDelay.emplace_back(newTimes[i] + delay);
	}


	delayTimes.insert(std::end(delayTimes), std::begin(timeDelay), std::end(timeDelay)); //Adds this new delayed vector to the old delay buffer

	int vecSize = newTimes.size(); //How many clicks did we just pull off the board?
	int fullSize = delayTimes.size(); // How many of : Clicks pulled off the board + clicks from delay buffer
	int delta = fullSize - vecSize; // Difference between number of clicks we just pulled and total number of clicks

	std::vector<std::bitset<5>>LateClicks = OldLateClicks; //Creates 5 bit vector for late clicks, including the buffer; DEFINITELY WORTH MAKING SURE THIS BEHAVES AS EXPECTED; I think we migth need to access the elements *inside* old late clicks?
	std::vector<std::bitset<5>>EarlyClicks = {}; //Creates 5 bit vector for early clicks

	for (int i = 0; i < delta; i += 1) {
		earlyTimes.emplace_back(0);			// We want Early and Late to be the same size for easy iteration; late is currently delta longer than early; adds delta elements to the front of Early to size(Late) = size(Early)
		EarlyClicks.emplace_back(0b00000);
	}

	earlyTimes.insert(std::end(earlyTimes), std::begin(newTimes), std::end(newTimes)); //stick the new times onto the back of the old times from the buffer to make our complete time bufffer.


	

	//LateClicks.insert(std::end(LateClicks), std::begin(OldLateClicks), std::end(OldLateClicks)); 


	std::bitset<5> LateClickMask(0b01000); //Creates a binary mask that looks for clicks on late channel

	std::bitset<5> EarlyClickMask(0b00010); //Creates a binary mask that looks for clicks on early channel

	//DEBUG UTILITY
	//for (int i = 0; i < vecSize; i += 1) {
		//std::cout << clicks[i] << std::endl;
	//}

	for (int i = 0; i < vecSize; i += 1) {
		LateClicks.emplace_back(clicks[i] & LateClickMask); //Add late clicks to late channel
		EarlyClicks.emplace_back(clicks[i] & EarlyClickMask); //Add early clicks to early channel
		//std::cout << LateClicks[i] << std::endl;
		//Note that at the end of this loop, EarlyClicks and LateClicks should still be the same size
	}

	for (int i = 0; i < fullSize; i+=1) {
		if (LateClicks[i] == 0b01000) {
			statsDelay[1] += 1; //check if count on Late Channel; if yes, add one to stats array
			if (EarlyClicks[i] == 0b00010) { //if also click on early channel, add a coincidnece
				//statsDelay[3] += 1;
				//std::cout << "Timestamp at" << delayTimes[i] << std::endl;
			}
		}
		if (EarlyClicks[i] == 0b00010) { //seperate routine for counting singles on early channel just for safety.
			statsDelay[2] += 1; //
		}
	}

/*
	std::pair<std::vector<int>, std::vector<int>> indices = findEqualElements(delayTimes, earlyTimes); //Finds what pair of indices describes equal elements

	removeDupes(indices.first);
	removeDupes(indices.second);

	int numEqualElements = indices.first.size(); //how many equal timestamps are there?

	//std::cout << (numEqualElements) << std::endl;
	//std::cout << fullSize << std::endl;

	for(int j = 0; j < numEqualElements; j+= 1){ //Checks statistics on each channel
		if (LateClicks[indices.first[j]] == 0b01000){
			if (EarlyClicks[indices.second[j]] == 0b00010) {
				std::cout << "Timestamp at" << delayTimes[indices.first[j]] << std::endl;
				statsDelay[3] += 1;
			}

			
			
			//std::cout << "difference:" << delayTimes[indices.first[j]] - earlyTimes[indices.first[j]] << std::endl;
			//std::cout << std::endl << LateClicks[indices.first[j]] << std::endl;
			//std::cout << std::endl << EarlyClicks[indices.second[j]] << std::endl;
			//statsDelay[3] += 1;
		}
	}
	*/
	//Now all we have to do is put the clicks that were delayed past the end of Early into a buffer for the next round!

	int lastEarly;
	if (!newTimes.empty()){
   		lastEarly = earlyTimes.back(); //what was the last timestamp in the early channel? By definition, this is the last timestamp for which the computer has recieved data from the board, so nothing with a later timestamp from the delayed channel can be correlated yet!
	}
	else{
		lastEarly = 0; //if empty, just set it to zero; *every timestamp in old is uncorrelated.
	}
	OldLateTimes = {};
	OldLateClicks = {};
	for(int i = 0; i < fullSize; i+=1){ //scans through the entire clicks/timestamps arrays
		if (delayTimes[i] >= lastEarly){ //If your timestamp in late is later than the last early timestamp...
			OldLateTimes.emplace_back(delayTimes[i]); //... then you go into the buffer!
			OldLateClicks.emplace_back(LateClicks[i]);//... and so do your clicks!
		}
	}

	return;
}

//End new correlation function!

//FPGA commands
static unsigned char FPGA_NO_CHANGE = 0;
static unsigned char FPGA_CLEAR = 1;
static unsigned char FPGA_DISABLE = 2;
static unsigned char FPGA_ENABLE = 4;
static unsigned char FPGA_GETDATA = 8;


//TTL FPGA output commands
static unsigned char FPGA_PULSE = 0x10; //16
static unsigned char FPGA_TOGGLE = 0x20; //32
//TTL channels
static unsigned char FPGA_TTL_1 = 0x40; //64
static unsigned char FPGA_TTL_2 = 0x80; //128

//Errors
static const int ERROR_USB_INITIATED = -1; //Error: USB communication already initiated.
static const int ERROR_USB_UNINITIATED = -2; //Error: Trying to close NULL USB connection.
static const int ERROR_FPGA_REQUEST_DATA_SIZE = -3; //Error while requesting data size from FPGA
static const int ERROR_FPGA_GET_DATA_SIZE = -4; //Error getting size of data from FPGA
static const int ERROR_FPGA_GET_BULK_DATA = -5; //Error while getting bulk data from FPGA
static const int ERROR_FPGA_DATA_SIZE = -6; //Error: returned data not the 64 bytes expected...
static const int ERROR_COUNTS_INITIALIZATION = -7; //Error: insufficiently sized buffer passed to FPGA_Counts()
static const int ERROR_FPGA_PULSE = -8; //Error: could not send pulse command to FPGA.
static const int ERROR_FPGA_TOGGLE = -9; //Error: could not send toggle command to FPGA.

//constants
static const int USBFrame = 512;
static const int BYTE_SIZE = 8;
static const int FPGAdataPointSize = 4; //Time Tag data is 4 bytes per click
static const int FPGAStatsDataSize = 64; //64 bytes

//KNJN GUID {0EFA2C93-0C7B-454F-9403-D638F6C37E65}
static GUID GUID_KNJN_FX2 = { 0x0EFA2C93, 0x0C7B, 0x454F, 0x94, 0x03, 0xD6, 0x38, 0xF6, 0xC3, 0x7E, 0x65 };
//CYUSB GUID="{AE18AA60-7F6A-11d4-97DD-00010229B959} //Using this one with the CyUSB signed driver!
static GUID GUID_CYUSB_FX2 = { 0xAE18AA60, 0x7F6A, 0x11d4, 0x97, 0xDD, 0x00, 0x01, 0x02, 0x29, 0xB9, 0x59 };

#define BulkOutPipe0 USBDevice->EndPoints[1] 
#define BulkInPipe1  USBDevice->EndPoints[2]
#define BulkOutPipe2 USBDevice->EndPoints[3]
#define BulkOutPipe3 USBDevice->EndPoints[4]
#define BulkInPipe4  USBDevice->EndPoints[5]
#define BulkInPipe5  USBDevice->EndPoints[6]


//Name of a function with real time access to received data.
//User could write his/her own function and give its name here. Default function is "correlate"
//The user function must be declared as
//void <function_name> (unsigned char* data, int length, __int64 *stats);
//where:
//data is an array of recorded events, 4 bytes are used per one event, as defined in documentation
//length is the length of the array (divide by 4 to get number of events)
//stats is an array of integers, with any user output. The pointer to zeroth element will be passed to LabVEIW. The LavVIEW code assumes a 16-element array.

//#define REALTIME_FUNCTION correlate


//#include "StatsDLL.h"

//Globally accessible elements. Attaching the DLL instantiates these objects, and detaching deletes them.
CCyUSBDevice* USBDevice;
unsigned char* buffer;
int bufferSize = 65536;
long moreData;//amount of data to retrieve from last transfer's overflow
int incompletePacket; //TODO: is this necessary?
const char* debug = "C:\\FPGA\\debug.txt";
std::ofstream debugLog; //Debug log only logs error statements in released code


//Open USB connection to FPGA board.
int USB_Open() {

	if (USBDevice == NULL) {
		//std::cout << "Creating USBDevice." << std::endl;
		//debugLog << "Creating USBDevice." << std::endl;
		USBDevice = new CCyUSBDevice(NULL, GUID_CYUSB_FX2, true);
		return 0;
	}
	else {
		//std::cout << "Error: USB communication already initiated." << std::endl;
		debugLog << "Error: USB communication already initiated." << std::endl;
		return ERROR_USB_INITIATED;
	}
}

//Close USB connection to FPGA board.
int USB_Close() {
	if (USBDevice == NULL) {
		//std::cout << "Error: Trying to close NULL USB connection." << std::endl;
		debugLog << "Error: Trying to close NULL USB connection." << std::endl;
		return ERROR_USB_UNINITIATED;
	}
	else {
		debugLog << "Closing and deleting USB Device." << std::endl;
		delete USBDevice;
		USBDevice = NULL;
		return 0;
	}
}

// Pulls timestamped clicks from FPGA, and calculates user-defined statistics data (stats)
int FPGA_TimeTag(bool saveClicks, unsigned char  fpgaCommand, char* fileName, __int64* stats, __int64* delayedStats, int runs, bool delay, int delayFrames, std::vector<int> timeBuff, std::vector<std::bitset<5>> clickBuff) {

	//debugLog << "start" << std::endl;
	//debugLog.flush(;)
	//debugLog << "Max packet size: " << USBDevice->MaxPacketSize << std::endl;

	//clicklog
	//std::string line;
	//static char delimiter = ' ';

	std::ofstream clickLog;
	if (saveClicks) {
		clickLog.open(fileName, std::ios::out | std::ios::app | std::ios::binary);
	}

	/*
	FILE * clickFile;
	if(saveClicks){
		clickFile = fopen(fileName ,"at");
		setvbuf(clickFile,NULL,_IOFBF,256);
	}
	*/
	//initialize stats?
	for (int i = 0; i < 16; i++) { //dangerous process... how do you know how long stats is?
		stats[i] = 0;
		delayedStats[i] = 0;
	}

	std::vector<int> timeLog = {}; //creates an empty array to keep the timestampls
	std::vector<std::bitset<5>> clicker = {}; // creates an empty array to keep the clicks

	//command to send to FPGA or data received from FPGA
	unsigned char fpgaCommunicator[2];
	long fpgaCommunicatorBytes = 1;
	fpgaCommunicator[0] = fpgaCommand;

	//variables
	int status = 0;
	long size = 0;

	for (int run = 0; run < runs; run++) {
		//request number of new data points
		if (!status) {
			fpgaCommunicator[0] = fpgaCommand;
			fpgaCommunicatorBytes = 1;
			///debugLog << "requesting size" << std::endl;
			status = !BulkOutPipe2->XferData(fpgaCommunicator, fpgaCommunicatorBytes);
			if (status) {
				debugLog << "error requesting data size" << std::endl;
				debugLog.flush();
				status = ERROR_FPGA_REQUEST_DATA_SIZE;
			}
		}
		if (!status) {
			//get data size
			fpgaCommunicator[0] = 0;
			fpgaCommunicator[1] = 1;
			fpgaCommunicatorBytes = 2;
			status = !BulkInPipe5->XferData(fpgaCommunicator, fpgaCommunicatorBytes);

			if (status) {//error
				debugLog << "error getting size of data" << std::endl;
				debugLog.flush();
				status = ERROR_FPGA_GET_DATA_SIZE;
			}
		}

		if (!status) {
			//calculate size
			size = (long)fpgaCommunicator[1] + ((long)fpgaCommunicator[0]) * 256; //number of bytes stored in FPGA since last query
			//debugLog << "data size: " << size << " + " << moreData << " from last round = " << size + moreData << std::endl;
			//debugLog.flush();
			size += moreData;



			if (size < USBFrame) {
				size = USBFrame;
			}
			moreData = size % USBFrame;
			size -= moreData;

			status = !BulkInPipe4->XferData((buffer + incompletePacket), size); //starting off at pointer + incompletePacket, to ensure that data point from last query is completed.
			
			if (status) {//error
				debugLog << "error sending data transfer request for size: " << size << std::endl;
				debugLog.flush();
				status = ERROR_FPGA_GET_BULK_DATA;;
			}
			else {
				size += incompletePacket; //the total data size in buffer is the size of the data retrieved + the incomplete packet from the previous query
				//debugLog << "got data of size: " << size << std::endl;
				//debugLog.flush();
			}
		}

		if (!status) {
			incompletePacket = size % FPGAdataPointSize;  //incomplete packets from this query
			if (incompletePacket) {
				debugLog << "Incomplete packet: " << incompletePacket << std::endl;
				debugLog.flush();
			}

			//save clicks, but don't save any incomplete data packets; save those for next round

			if (saveClicks) {
				for (int j = 0; j < (size - incompletePacket); j += FPGAdataPointSize) {
					//big-endian binary output with whitespace
					//clickLog << (unsigned int) buffer[j+3] << delimiter << (unsigned int) buffer[j+2] << delimiter << (unsigned int) buffer[j+1] << delimiter << (unsigned int) buffer[j] << std::endl;

					// FPGA outputs little-endian, which is what is used on x86; we output little-endian with no whitespace for compactness
					clickLog << buffer[j] << buffer[j + 1] << buffer[j + 2] << buffer[j + 3];

					//writing the start, ch 1, ch 2, ch 3, ch 4 as a hexadecimal, then the timestamp as an integer.
					//clickLog << std::hex << std::showbase << (buffer[j+3]&248) << delimiter;

					//writing the start, ch 1, ch 2, ch 3, ch 4 as a pesudo-binary string, then the timestamp as an integer.
					//clickLog << (std::bitset<5>) ((buffer[j+3]&248) >> 3) << delimiter;
					//clickLog << std::dec << (buffer[j+3]&7)*256*256*256+buffer[j+2]*256*256+buffer[j+1]*256+buffer[j] << std::endl;


					//faster write?

					// this is the pseudo-binary for clicks/starts, and integer timestamp
					/*
					line = ((std::bitset<5>) ((buffer[j+3]&248) >> 3)).to_string();
					line += ' ';
					line += std::to_string((buffer[j+3]&7)*256*256*256+buffer[j+2]*256*256+buffer[j+1]*256+buffer[j]);
					line += '\n';
					line.shrink_to_fit();
					fwrite(line.c_str(), sizeof(char),line.length(),clickFile);
					*/
					//faster write for pure binary
					//fwrite(&buffer[j],sizeof(buffer[j]),FPGAdataPointSize,clickFile);


					//casting each byte as a bitset to print in "binary" or a string of 1s and 0s.
					//clickLog << (std::bitset<8>) buffer[j+3] << delimiter << (std::bitset<8>) buffer[j+2] << delimiter << (std::bitset<8>) buffer[j+1] << delimiter << (std::bitset<8>) buffer[j] << std::endl;
				}
				//clickLog.flush();
			}

			if (delay) { //Starts 'delay section'
				//std::cout << "Start!" << std::endl; //debug feature; prints start

				
				std::bitset<32> timeDivide(0b11111111111111111111111111100000); // creates a binary mask that can be used to take only the 27 bit timestamp piece of the returned data
				std::bitset<32> clickDivide(0b00000000000000000000000000011111); // creates a binary mask that can be used to take only the 5 bit click piece of the returned data
				for (unsigned int a = 0; a < (size - incompletePacket); a = a + 4) { //starts a loop to run through and decode the buffer; each 4 bytes of buffer data represent 1 event
					
					//int state[4];
					//state[0] = (buffer[a + 3] & 0x08) ? true : false;
					//state[1] = (buffer[a + 3] & 0x10) ? true : false;
					//state[2] = (buffer[a + 3] & 0x20) ? true : false;
					//state[3] = (buffer[a + 3] & 0x40) ? true : false;

					
					
					//for (int i = 0; i < 4; i += 1) {
					//	std::cout << state[i];
					//}
					//std::cout << '\n';
					
					std::bitset<sizeof(char)* CHAR_BIT> binary(reverse(buffer[a])); //decodes the first byte into binary
					int bin1;
					bin1 = (int)(binary.to_ulong()); // turns it into int
					std::bitset<sizeof(char)* CHAR_BIT> binary2(reverse(buffer[a+1])); //decodes second byte into binary
					int bin2;
					bin2 = (int)(binary2.to_ulong()); //turns it into int
					std::bitset<sizeof(char)* CHAR_BIT> binary3(reverse(buffer[a + 2])); //decodes third byte into binary
					int bin3;
					bin3 = (int)(binary3.to_ulong()); // turns it into int
					std::bitset<sizeof(char)* CHAR_BIT> binary4(reverse(buffer[a + 3])); //decodes fourth byte into binary
					int bin4;
					bin4 = (int)(binary4.to_ulong()); // turns it into int
					uint32_t result = 0;
					result += (bin4 << 0); //combines the four ints into one number 32 bit number that preserves the binary structure
					result += (bin3 << 8);
					result += (bin2 << 16);
					result += (bin1 << 24);
					//std::cout << binary<<binary2<<binary3<<binary4 << std::endl;
					//std::cout << std::bitset<32>(result) << std::endl;
					std::bitset<32> timeStampAndClicks(result); // re-encodes the result into binary
					std::bitset<27> timeStamp((timeStampAndClicks& timeDivide).to_ulong()); //takes the timestamp piece



					reverseBit(timeStamp);



					std::bitset<5> clicks((timeStampAndClicks& clickDivide).to_ulong()); //takes the event code piece

					//std::cout << timeStamp << std::endl;
					//std::cout << clicks << std::endl;
					timeLog.emplace_back(timeStamp.to_ulong()); //adds the timestamp to the timeLog array
					clicker.emplace_back(clicks); //adds the clicks to the click Log array.

				}
				//std::cout << "Ran delay!" << std::endl;
		//********* DEBUGGING UTILITIES **********************
				//std::cout << "There are " << bufferSize / sizeof(buffer[0]) << " elements in the buffer" << std::endl;
				//for (std::vector<int>::const_iterator i = timeLog.begin(); i != timeLog.end(); ++i)
				//	std::cout << *i << '\n';
				//std::cout << "Priting Clicker Before Func:";
				//for (std::vector<std::bitset<5>>::const_iterator i = clicker.begin(); i != clicker.end(); ++i)
					//std::cout << *i << '\n';
				//std::cout << "Done!" << std::endl;
//*******************************************************************************
				
			}

			if (incompletePacket) {
				for (int k = 0; k < incompletePacket; k++) {
					buffer[k] = buffer[size + k - incompletePacket]; //transfer the bits from the incomplete data packet to the beginning of the buffer
				}
			}
		}

		//stats
		
		REALTIME_FUNCTION(buffer, size, stats);
		REALTIME_FUNCTION_2(timeLog, clicker, delayedStats, delay, timeBuff, clickBuff);
		timeLog = {};
		clicker = {};

		
	}

	//debugLog << "stop" << std::endl;
	//debugLog.close();


	if (saveClicks) {
		clickLog.close();
	}
	/*
	if(saveClicks){
		fclose(clickFile);
	}*/

	return status;
}



// Pulls click statistics from the FPGA

int FPGA_Counts(bool saveData, unsigned char  fpgaCommand, char* fileName, __int64* stats, unsigned __int64* data, int* length, int runs) {

	//datalog
	static char delimiter = ' ';
	std::ofstream dataLog;
	if (saveData) {
		dataLog.open(fileName, std::ios::out | std::ios::app);
	}

	//command to send to FPGA or data received from FPGA
	unsigned char fpgaCommunicator[2];
	long fpgaCommunicatorBytes = 1;
	fpgaCommunicator[0] = fpgaCommand;

	//variables
	int status = 0;
	long size = 0;

	//if an insufficiently large data array is passed, fail before getting any data at all...
	if (runs * FPGAStatsDataSize > *length) {
		status = ERROR_COUNTS_INITIALIZATION;
	}
	else {
		*length = 0;
	}

	for (int run = 0; run < runs; run++) {

		//request number of new data points
		if (!status) {
			fpgaCommunicator[0] = fpgaCommand;
			fpgaCommunicator[1] = 0;
			fpgaCommunicatorBytes = 1;
			///debugLog << "requesting size" << std::endl;
			status = !BulkOutPipe2->XferData(fpgaCommunicator, fpgaCommunicatorBytes);

			if (status) {
				debugLog << "error requesting data size" << std::endl;
				debugLog.flush();
				status = ERROR_FPGA_REQUEST_DATA_SIZE;
			}
		}
		if (!status) {
			//get data size --NOTE: This isn't really necessary, since we know the data will be 64 bytes. This was just to reduce development time by using the TimeTag project as a template.
			fpgaCommunicator[0] = 0;
			fpgaCommunicator[1] = 1;
			fpgaCommunicatorBytes = 512;
			status = !BulkInPipe5->XferData(buffer + incompletePacket, fpgaCommunicatorBytes);

			if (status) {//error
				debugLog << "error getting size of data" << std::endl;
				debugLog.flush();
				status = ERROR_FPGA_GET_DATA_SIZE;
			}
		}

		if (!status) {
			//calculate size
			size = (long)(buffer + incompletePacket)[1] + ((long)(buffer + incompletePacket)[0]) * 256; //number of bytes stored in FPGA since last query
			//debugLog << "data size: " << size << " + " << moreData << " from last round = " << size + moreData << std::endl;
			//debugLog.flush();
			size += moreData;

			if (size < USBFrame) {
				size = USBFrame;
			}
			moreData = size % USBFrame;
			size -= moreData;

			status = !BulkInPipe4->XferData((buffer + incompletePacket), size); //starting off at pointer + incompletePacket, to ensure that data point from last query is completed.

			if (status) {//error
				debugLog << status << "error requesting/getting data of size: " << size << std::endl;
				debugLog.flush();
				status = ERROR_FPGA_GET_BULK_DATA;
			}
			else {
				size += incompletePacket; //the total data size in buffer is the size of the data retrieved + the incomplete packet from the previous query
				if (size != 64) {
					//this really ought not happen...
					status = ERROR_FPGA_DATA_SIZE;
					debugLog << status << " Error: Wrong data size; retrieved data of size: " << size << std::endl;
					debugLog.flush();
				}
				//debugLog << "got data of size: " << size << std::endl;
				//debugLog.flush();
			}
		}

		if (!status) {
			incompletePacket = size % BYTE_SIZE;  //incomplete packets from this query... don't think there should be any.
			if (incompletePacket) {
				debugLog << "This shouldn't happen? Incompete packet... size%8 != 0..." << std::endl;
				debugLog.flush();
			}


			//save data, but don't save any incomplete data packets; save those for next round, if they exist at all...

			//generate stats output
			//this uses knowledge of the byte by byte structure of the data returned by the FPGA
			//6 bytes for the first set (clocks), 4 bytes each for clicks, two-, and three-fold coincidences, and 2 bytes for 4-fold coincidences. This totals 64 bytes.
			//The data seems to be least-significant byte first due to how it was streamed from the FPGA

			//first six bytes of 0 clicks
			unsigned int p = 1;
			for (int k = 0; k < 6; k++) {
				stats[0] += (int)p * ((unsigned int)buffer[k]);
				p = p << 8; //next byte should be multiplied by a number 2^8 greater
			}
			//next 56 bytes:
			for (int j = 6; j < 62; j += 4) {
				p = 1; //2^(3*8) is t he amount by which the 4th byte needs to be raised
				for (int k = 0; k < 4; k++) {
					stats[(j - 2) / 4] += p * ((unsigned int)buffer[j + k]);
					p = p << 8; //next byte should be multiplied by a number 2^8 greater
				}
			}
			//last two bytes
			stats[15] = (unsigned int)buffer[62] + 256 * ((unsigned int)buffer[63]);

			//Now to store the data
			for (int j = 0; j < (size - incompletePacket); j += BYTE_SIZE) {
				for (int k = 0; k < BYTE_SIZE; k++) {
					*(data + (*length) + j + k) = buffer[j + k + incompletePacket];
					if (saveData) {
						dataLog << buffer[j + BYTE_SIZE - k - 1] << delimiter;
					}
				}
				if (saveData) {
					dataLog << std::endl;
				}
			}
			if (saveData) {
				//debugLog << "data saved." << std::endl;
				//debugLog.flush();
				dataLog.flush();
			}
			*length += (size - incompletePacket);
			//debugLog << "Length: " << *length << std::endl;

			if (incompletePacket) { //really don't think this should happen... but just in case?
				for (int k = 0; k < incompletePacket; k++) {
					buffer[k] = buffer[size + k - incompletePacket]; //transfer the bits from the incomplete data packet to the beginning of the buffer
				}
			}
		}
	}

	//debugLog << "stop" << std::endl;
	//debugLog.close();

	if (saveData) {
		dataLog.close();
	}

	return status;
}


/********
Output pulse from channel.
returns 0 if successful, negative value if error
********/

int FPGA_Pulse(unsigned char fpgaTTLChannel) {

	//command to send to FPGA or data received from FPGA
	unsigned char fpgaCommunicator[2];
	long fpgaCommunicatorBytes = 1;
	fpgaCommunicator[0] = fpgaTTLChannel | FPGA_PULSE;
	fpgaCommunicator[1] = 0;

	//variables
	int status = 0;
	long size = 0;

	//send a command to the fpga
	status = !BulkOutPipe2->XferData(fpgaCommunicator, fpgaCommunicatorBytes);

	if (status) {
		debugLog << "error requesting pulse" << std::endl;
		debugLog.flush();
		status = ERROR_FPGA_PULSE;
	}

	return status;
}


/********
Toggle channel output state.

returns 0 if successful, or negative value if error
********/

int FPGA_Toggle(unsigned char fpgaTTLChannel) {
	//command to send to FPGA or data received from FPGA
	unsigned char fpgaCommunicator[2];
	long fpgaCommunicatorBytes = 1;
	fpgaCommunicator[0] = fpgaTTLChannel | FPGA_TOGGLE;
	fpgaCommunicator[1] = 0;

	//variables
	int status = 0;
	long size = 0;

	//send a command to the fpga
	status = !BulkOutPipe2->XferData(fpgaCommunicator, fpgaCommunicatorBytes);

	if (status) {
		debugLog << "error requesting toggle" << std::endl;
		debugLog.flush();
		status = ERROR_FPGA_TOGGLE;
	}

	return status;
}

/////////////////////////////////////////////////////
// Default real-time
// Simple correlation analysis. Computes statistics.
/////////////////////////////////////////////////////
/*
stats structure: number of occurences of these events:
 stats[0] start events
 stats[1] state 0
 stats[2] state 1
 stats[3] state 2
 stats[4] state 3
 stats[5] states 0&1
 stats[6] states 0&2
 stats[7] states 0&3
 stats[8] state 1&2
 stats[9] state 1&3
 stats[10] state 2&3
 stats[11] states 0&1&2
 stats[12] states 0&1&3
 stats[13] states 0&2&3
 stats[14] state 1&2&3
 stats[15] states 0&1&2&3

*/






void streamData() {
	//std::cout << "Pass 1!";
    int USBstatus;
	//std::cout << "Pass 2!";

	bool saveClicks = false;  //you want to save time - tagged events to a file
	//std::cout << "Pass 3!";
    unsigned char fpgaCommand = FPGA_ENABLE;  // Set the desired FPGA command
	//std::cout << "Pass 3!";
    char* fileName;  // Specify the file name for saving time-tagged events
    __int64 stats[16];  // Array to store the coincidence statistics
	__int64 delayedStats[16];
	std::vector<int> timeBuff = {};
	std::vector<std::bitset<5>> clickBuff = {};
	std::cout << "Successfully built delay buffers" << std::endl;

    int runs = 100;  // Number of times to perform USB transfer before returning the stats

	char hello[] = "clickLog.txt";
    fileName = hello;


    USBstatus = USB_Open();
	std::cout << "Successfully opened USB" << std::endl;
	//std::cout << USBstatus;
	//std::cout << "Pass 4!";
    if (USBstatus != 0) {
		std::cout << ("USB Protocol Failure");
    }
    int result = FPGA_TimeTag(saveClicks, fpgaCommand, fileName, stats, delayedStats, runs, 0, 1, timeBuff, clickBuff);
	std::cout << "Successfully ran one loop" << std::endl;
	//std::cout << result;
    fpgaCommand = FPGA_NO_CHANGE;

    while (true) {
        result = FPGA_TimeTag(saveClicks, fpgaCommand, fileName, stats, delayedStats, runs, 1, 1, timeBuff, clickBuff);
		std::cout << "Ran two loops!" << std::endl;
        if (result) {
			std::cout << "Entered Error Sequence" << std::endl;
            // Handle errors if needed
            // For example, print an error message
			std::cout << ("FPGA_TimeTag error: %d\n", result);
            break;  // Terminate the loop in case of error
        }
        // Print the stats to stdout

		std::cout << "Delayed Output:";

        for (int i = 0; i < 16; i++) {
			//std::cout << stats[i] << ",";
			std::cout << delayedStats[i] << ",";
        }
		std::cout << ("\n");
		std::cout << "Normal Output:";
		for (int i = 0; i < 16; i++) {
			std::cout << stats[i] << ",";
			//std::cout << "Delayed Output:" << delayedStats[i] << ",";
		}
		std::cout << ("\n");

        // Flush stdout to ensure immediate printing
        fflush(stdout);

        // Delay for a certain period before the next iteration
        // Adjust the delay as per your requirements
        // Example: sleep for 1 second
        //sleep(1);
    }
}


int main() {
	std::cout << "Hello World!" << std::endl;
	debugLog.open(debug, std::ios::out | std::ios::app); //only errors enter the debug log
	USBDevice = NULL;
	buffer = new unsigned char[bufferSize];
	moreData = 0;
	incompletePacket = 0;
	//debugLog << "attaching DLL" << std::endl;
	//debugLog.flush();
	std::cout << "Start datastream!" <<std::endl;
    streamData();
	delete USBDevice;
	delete buffer;
	//debugLog << "detaching DLL" << std::endl;
	debugLog.close();

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
