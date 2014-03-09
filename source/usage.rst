==============
Using kodiaq
==============

There are 3 sub-windows in the DAQ UI: control, monitoring, and
history. The following is a basic explanation of the information
provided and interactions possible with each.

DAQ Status window
------------------

The DAQ status window is on the right side of the screen and is split
into two parts. The top part gives general information on the state of
the overall DAQ while the bottom part gives information on the status
of each connected slave. We will start from the top down.

* Network *(Disconnected/Connected)*: 
    The DAQ network is the connection of the master program to the slave PCs. If the network
    interface is up and at least one slave is connected this field will
    read "Connected". 
* Acquisition *(Idle/Armed/Running/Mixed)*: 
    This tells if the DAQ is taking data or not. If the boards are not doing anything they are
    in idle state. The DAQ cannot be started in idle state. In order to
    start the DAQ the user has to choose a run mode. Once a mode is chosen
    the run mode is sent to the slave modules where mode-specific options
    are loaded into the digitizers. If all slaves successfully load the
    options the DAQ is in armed mode and can be started. If the DAQ is
    taking data it is in running mode, stopping the DAQ puts it back to
    armed mode. If all slave modes are not reporting the same mode the DAQ
    is in mixed mode. This is normal for a brief amount of time upon a state
    change but should not persist for more than a couple seconds.
* DAQ Mode *(a string)*:
    Gives the name of the run mode used to collect the most recent run.
* Current Run *(run name)*: 
    Gives the name of the current or most recently stopped run.
* Started by *(user name)*: 
   Gives the name of the user who started the current or most recently stopped run.
   
The lower part of the window provides information on each slave
connected. The color of the slave name indicates its state (red-idle,
yellow-armed, green-running). The rate given is the rate data is read
out of the readout buffers of the digitizers. Due to formatting and
zipping this is different than the rate actually transferred to the
event builder buffer. The number of boards given is simply the number
of digitizers connected to that slave. The frequency is the frequency
of reads from the readout buffers of the digitizer. It is possible for
multiple occurrences to be read in a single buffer. Note that this has
nothing to do with the frequency of events since the concept of an
event does not exist at this point in the data stream.

Control Window
------------------

The control window is on the top left and allows the user to control
the DAQ. The options provided are dependent on the current state of
the DAQ. The following table describes what each option does and under
what states the option is available.

+------------------+--------------------+-----------------------------------------+
| Command          | States             | Action                                  |
+==================+====================+=========================================+
| Connect          | DAQ Disconnected   | | Puts up the DAQ network. The          |
|                  |                    | | master will put up its interface      |
|		   |			| | and wait for 10 seconds for slaves to |
|                  |                    | | connect                               |
+------------------+--------------------+-----------------------------------------+
| Broadcast Message| All                | | Puts a message into the DAQ log and   |
|                  |                    | | broadcasts it to all connected UIs    |
+------------------+--------------------+-----------------------------------------+
| Choose Operation | DAQ Connected      | | Asks the master for a list of         |
| Mode             |                    | | available run modes. The user can     |               
|                  |                    | | then choose a mode from the list.     | 
|                  |                    | | the chosen mode is shipped to the     |
|                  |                    | | slaves and the boards are armed.      |
+------------------+--------------------+-----------------------------------------+
| Put DAQ into     | DAQ Armed          | | Clears the last run mode and puts the |
| Standby Mode     |                    | | boards into the idle state. Use this  |
|                  |                    | | if you won't use the DAQ for a bit    |
+------------------+--------------------+-----------------------------------------+
| Reset DAQ        | Idle               | | Disconnects and reconnects the DAQ    |
|                  |                    | | network. Used to debug connection     |
|                  |                    | | issues.                               |
+------------------+--------------------+-----------------------------------------+
| Start DAQ        | Armed              | | Starts acquisition into a new run.    |
+------------------+--------------------+-----------------------------------------+
| Stop  DAQ        | Running            | | Stops acquisition.                    |
+------------------+--------------------+-----------------------------------------+
| Quit             | Running            | | Quit out of the UI. This does not     |
|                  |                    | | have any effect on the DAQ itself.    |
+------------------+--------------------+-----------------------------------------+

Messages Window
------------------

The messages window displays the last 1000 lines of the DAQ log file
as well as any messages for the user. Scrolling is done with the arrow
keys or the mouse wheel. In order to place a message in the log file
use the "Broadcast Message" command. If you want to see something
older than 1000 lines you have to look up the actual DAQ log files. 