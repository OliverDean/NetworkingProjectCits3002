please read 
https://beej.us/guide/bgnet/html/index-wide.html
by the weekend

look through netcat and wireshark as useful tools as suggested by James(TA) and Karla

Question bank stores the questions and answers in a plain text file
Store the questions as different files maybe q1 is a text file, q2 is a picture.
each student must have a random set of 10 questions that is seeded such that they can come back at any time and see the same question order

The TM requests a set of questions from the qb
The TM will be written in JAVA
The TM stores all the login information for the people

The QB will be written in Python
the QB needs to use system calls as directed in Beej's such that it is language independent.

Need to keep the same 3 ip addresses for the computers, currently unknown as to how
Ipv6 suggested

struct for each student, int for the last question visited


initially start without using any networking, make sure that the core application works first.

leave the client side web app to last as it is the easiest with just a bit og html and css
