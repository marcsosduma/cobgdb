<h1>COBGDB</h1>

It is a command-line application, programmed in C, designed to assist in debugging GnuCOBOL code using GDB.
The application is based on the extension for VSCODE by master Oleg Kunitsyn, which can be found on GitHub: https://github.com/OlegKunitsyn/gnucobol-debug.
COBGDB is still in development.
To compile the code on Windows, MinGW and the C regular expression libraries available on the MinGW website were used.
The Makefile is configured to generate the program for both Windows and Linux.

To run the example program:

1. On Windows, install MinGW and the REGEX library (contributions from MinGW).
2. Execute the "make" command to compile the code.
3. Run the example program:
   cobgdb customer.cob

On Linux, it is recommended to use Xterm to view the application.

COBGDB running:


![Screenshot](cobgdb_run.png)


Debugging application output:

![Screenshot](customer_run.png)
