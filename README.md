# Messenger Assignment

### Brief
This project is implementation of simple message buffering and parsing. Message consists of name and message length, whose max lenghts are 15 and 31 respectively.

### Message packet structure
```
+-MSG packet------------------------------------------------------------------------------------------>
0        2 3          6 7           11 12     15 16        
+---------+------------+--------------+---------+----------------------+------------------------------+
|  FLAG   | NAME_LEN   |    MSG_LEN   |  CRC4   |        NAME          |            MSG               |
+---------+------------+--------------+---------+----------------------+------------------------------+
```
#### Fields descriptions
<b>FLAG</b> (3 bits) - A constant flag, used as signature to denote start of packet<br>
<b>NAME_LEN</b> (4 bits) - length of name (max 15)<br>
<b>MSG_LEN</b> (5 bits) - length of message (max 31)<br>
<b>CRC4</b> (4 bits) - CRC4 of packet (calculation of which is questionable, and really tied to this implementation)<br>
<b>NAME</b> (1 - 15 bytes) - Sender's name <b>(May not be empty)</b><br>
<b>MSG</b> (1 - 31 bytes) - Sender's message <b>(May not be empty)</b><br>

### Notes

#### Parsing
buffer may contain multiple message packets, which, when parsed, are presented in single object

#### CRC4
may not be compatible with other implementations of packet buffering/parsing

#### Exceptions are thrown, if
##### While buffering packet
* name is empty
* name exceeds max length (>15 bytes)
* text is empty

##### While parsing buffer
* flag bits are invalid
* name and message exceed packet length (indicated by flags)
* name is empty (indicated by flag)
* message is empty (indicated by flag)
* invalid CRC4
* packet does not contain enough bytes for packet Should contain atleast as much as Header Size (2 bytes)
* buffer's packets' have inconsistent sender's name (it is not the same across packets) 

### External software
Build using CMake<br>
Tested with Catch2
