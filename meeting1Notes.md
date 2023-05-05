please read 
https://beej.us/guide/bgnet/html/index-wide.html

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
note- facilitator suggests using ipv4 as there is significantly more information online for it.

struct for each student, int for the last question visited


initially start without using any networking, make sure that the core application works first.

leave the client side web app to last as it is the easiest with just a bit og html and css


clarification for criterion #4; (Karla)
*your system will execute two distinct QB instances, with each QB supporting (generating and assessing) questions from a single programming language. However, you should develop only a single QB application, able to support multiple languages.*

there should be one function that should be able to handle the answers from the user and compare them to the correct answers stored in the QB.
So for the coding questions, that one function should be compiling the user's answers regardless of the language it is written in, and then comparing the result to the answers stored in the QB.

1. There are  2 directories - C, Java and/or Python. 
2. A user gets a random coding question from let's say the Python QB. 
3. The user then writes their code answer in Python, the TM "captures" it as text, and then forwards it to the QB program that checks our answers. 
4. There is only one QB program that needs to be able to handle coding questions in all languages --> So there should be a single QB application, as the constraint says, that can support multiple languages. 

In regards to "Cause there's gonna need to be a way to define what language the person is typing in" --> we can, in the question description, say: "Write a Python function named __my_function__ that adds two numbers together". Therefore we are only accepting Python answers. I don't think we need to have checks making sure they are writing in the specified language.

