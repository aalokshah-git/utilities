While dealing with Analog world, firmware enginners have to play with ADC sample points, and it is not that easy as it sounds!
Usually hardware is preferred to be well-designed to take care of filters, but there are times when firmware need to do that to enhance or overcome against the hardware design.

About Utility:
This utility takes 'ADC RAW Sample Points' as input, apply firmware Filters and FFT on top, and plots them in XLSX output file.
This utility can save a lot of time during R&D, while deriving some algorithm based on input data.
It was helpful in my team while working on IIOT/ML Smart Sensor Devlopment. 

====================================================
Steps to follow on Linux:

1> Install 'libxlsxwriter' following the link - https://libxlsxwriter.github.io/getting_started.html

2> Compilation steps:
	a) Check the path of the installed library
		"sudo find / -name libxlsxwriter.so"
	b) Update the environment variable against the path found
		LD_LIBRARY_PATH=/usr/local/lib
		export LD_LIBRARY_PATH
	c) Compile - "gcc main.c kiss_fft.c -lxlsxwriter -lm -o filter_fft_generation"

3> Run './filter_fft_generation'
	-> It will take "input.csv" as input and generate "output.xlsx"

====================================================
Notes:

1> FFT:
	-> Opensouce KissFFT library is used for FFT (http://kissfft.sourceforge.net/) 

2> FILTER:
	-> Butterworth Highpass, Lowpass and Bandpass filter code is generated using http://www-users.cs.york.ac.uk/~fisher/mkfilter/trad.html
	-> Similar filter code generation options are available which can be used and functions in this utiliy can be replaced to get desired outputs
