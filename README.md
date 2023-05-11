# PhotonOS: A Flexible Platform For Entangled Photon Experiments
By Callum Welsh, based on the absolutely phenomenal work of [Sergey Polyakov, Alan Migdall, and Sae Woo Nam at NIST](https://www.nist.gov/services-resources/software/simple-and-inexpensive-fpga-based-fast-multichannel-acquisition-board). Developed for [Chin Lab at The University of Chicago](https://ultracold.uchicago.edu/)

## FPGA Data Readout straight to a Python GUI
For me, at least, NI Labview is frustrating and unintuitive. This software is intended to provide a way to read out time tagged data from Single Photon Counting Modules to a Python GUI that is 1) easily reconfigurable to suit whatever needs a user wants and 2) puts all of the tools used for Quantum State Tomography in one place.

This work (for now) requires you to use a Xylo-EM dev board with an Altera Cyclone II FPGA.

## This project is under development. Immediate Development Goals:
1. Establish a link between a Xylo-EM FPGA development board and a Python GUI that can readout data asynchronously and in real time.
2. Create a tool that lets us save data and run statistics on it during an experiment.
3. Connect these tools to automated motorized waveplate controls
4. Build a tool where you click "Tomography" and a rigorous, automated Quantum State Tomography meassurement is preformed.


If you work in this field and are seeing this, don't hesitate to reach me at callumwelsh@princeton.edu if you have any requests, questions, grievances etc.
