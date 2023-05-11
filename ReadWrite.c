#include <stdio.h>
#include "StatsDLL.h"


void streamData() {
    int USBstatus;


    bool saveClicks = false;  // Set to true if you want to save time-tagged events to a file
    unsigned char fpgaCommand = FPGA_ENABLE;  // Set the desired FPGA command
    char* fileName = "time_tagged_events.txt";  // Specify the file name for saving time-tagged events
    __int64 stats[16];  // Array to store the coincidence statistics
    int runs = 1;  // Number of times to perform USB transfer before returning the stats

    USBstatus = USB_Open();
    if (USBstatus != 0) {
        printf("USB Protocol Failure");
        break;
    }

    int result = FPGA_TimeTag(saveClicks, fpgaCommand, fileName, stats, runs);
    fpgaCommand = FPGA_NO_CHANGE;
    
     while (true) {
        int result = FPGA_TimeTag(saveClicks, fpgaCommand, fileName, stats, runs);
        if (result < 0) {
            // Handle errors if needed
            // For example, print an error message
            printf("FPGA_TimeTag error: %d\n", result);
            break;  // Terminate the loop in case of error
        }

        // Print the stats to stdout
        for (int i = 0; i < 16; i++) {
            printf("%lld ", stats[i]);
        }
        printf("\n");

        // Flush stdout to ensure immediate printing
        fflush(stdout);

        // Delay for a certain period before the next iteration
        // Adjust the delay as per your requirements
        // Example: sleep for 1 second
        //sleep(1);
    }
}


int main() {
    streamData();

    return 0;
}