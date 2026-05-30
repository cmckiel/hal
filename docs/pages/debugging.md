# On-Target Debugging

This set of instructions gets a developer up and running with debugging a live target.

## Setup

The setup assumes a raspberry pi or other device as a bridge between the development computer and the target.

![Debugging Setup](RemoteDebugDiagram.svg)

From the development computer, open up an SSH connection like so:

`ssh -L 3333:localhost:3333 user@host`

This forwards your dev machine's port 3333 to the remote's localhost 3333 port. This is the port over which GDB Remote Serial protocol communicates with the debug server running on Pi.

Using the same session just opened, execute the following command:

`openocd -f interface/stlink.cfg -f target/stm32f4x.cfg`

This starts the openocd server, which listens on localhost 3333 for GDB RSP commands and manages the interaction with the target hardware to enact them.

## Starting a Session

Navigate to the debugging sidebar and select the `Debug STM32F4 (via Pi tunnel)` option from the debug configuration dropdown. Clicking the run button should start a session and look something like this:

![Debug Session](UART_DataLoss_2ms_LiveDebug.png)
