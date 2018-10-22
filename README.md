# Digit-control-on-7-segment
The user space program initiates a system call to the driver implemented in the kernel. Based on the input provided by user space process, the driver controls four digits of the 4 digit seven segment display.

Drivers provide a software interface that allows the OS to communicate and control hardware devices. Building a device driver by implementing a driver that controls the input and output of some physical hardware. More specifically, the driver will control some lights (LEDs) and a button. A user space program will initiate a system call to the driver implemented in the kernel. 
Based on the input provided by user space process, the driver will turn will control four-digit 7 segment display. 
The driver will also handle hardware signals, but turning off the segment whenever a physical button is pressed.

Hardware Required: 
  a) Beagle Bone Black
  b) Breadboard to build your circuit. 
  c) 4 digit 7 segment display
  d) A button and wires 
 
Task:
1. Connect the 7 segment display to the BBB
2. Create a user-level program passes a string containing 4 bits of information (for example “1001” or “0111”. The user-level program should continue to prompt the user until -1 is entered.
3. The driver should display the binary number on the 7 segment display. For example, if the number “1111” is sent, then all the digits display "1111". If the number “0111” is transmitted, only three digits should be turned ON.
4. When the button is pressed, it should act as a reset. In other words, when the button is pressed, all the digits should be turned OFF until another number is transmitted from the user space program.
5. Avoiding race conditions. Two user space programs should never be able to have the driver open at the same time.
