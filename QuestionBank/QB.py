import random
import os
import socket
import string
import sys, getopt
import select

answered_questions_file = "answered_questions.txt"

# Define your TM server credentials here
TM_SERVER = "192.168.0.195"
PQB_PORT = 4126
CQB_PORT = 4127
PQB = "PythonQuestionBank"
CQB = "CQuestionBank"

def communicate_with_tm(s):
    # Receive the response from the TM server
    while true:
        data = s.recv(2)
        if data.decode() == "GQ":
            print("generating questions\n")
            questions = generate_questions()
            combined = ''.join(questions)
            s.send(combined.encode())
        elif data.decode() == "exit":
            print("Exit command received. Closing connection.")
            break
            
    # Decode the received data and return it

def generate_questions():
    questions = []
    for x in range(4): #generate 4 random questions
        flag = 0
        print(x)
        questionID = random.choice(string.ascii_lowercase) #generate random question ID
        print("question id is:\n")
        print(questionID)
        arraylength = len(questions)
        for y in range(arraylength):
            if questions[y] == questionID: #if ID has already been generated successfully, generate a new ID
                x -= 1
                flag += 1
                break
        if flag >= 1:
            continue
        buffer = grab_questionID(questionID) #grab question ID if it exists
        if buffer == "no": #if index wasn't successfully found
            x -= 1
            continue
        questions.append(buffer)
        questions.append(';')
    return questions

def grab_questionID(questionID):
    with open(CQB, "r", encoding="utf-8") as f:
        lines = f.read().splitlines()
    i = 0
    while i < len(lines):
        if not lines[i].strip():
            i+=1
            continue
        buff = lines[i].strip()
        if buff != questionID:
            i+=1
            continue
        print(questionID)
        print("Found question id")
        print(buff)
        return buff
    return "no"

def connect_to_tm(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((TM_SERVER, port))
    print("connected")
    return s
  
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "pc")
    except:
        print ("select one\n -P for python\n -C for C")
        sys.exit(2)
    print("checking...")
    for opt, arg in opts:
        if opt == '-p': #python
            print("conencting to python")
            s = connect_to_tm(PQB_PORT)
            communicate_with_tm(s)
        elif opt == '-c': #c
            print("conencting to c")
            s = connect_to_tm(CQB_PORT)
            communicate_with_tm(s) 
    
if __name__ == "__main__":
    main()