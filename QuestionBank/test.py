import random
import subprocess
import os
import string
import socket
import sys, getopt

TM_SERVER = "192.168.0.195"
PQB_PORT = 4126
CQB_PORT = 4127
PQB = "PythonQuestionBank"
CQB = "CQuestionBank"

def questiontest():
    # quetion bank for testing
    question_bank = {
        "Python: write a function and return the total of two input number": ((5, 10), 15, "python", lambda a, b: a + b),
        "C: write a function and return the total of two input number ": (("5", "10"), "15\n", "c", "int main(){int a=5; int b=10; printf(\"%d\", a+b); return 0;}"),
        # add more question
    }

    # select random question from bank
    question, (input_args, expected_output, lang, solution) = random.choice(list(question_bank.items()))

    print(f"Question: {question}")
    function_name = input("write your function name: ")
    user_code = input("write your code: ")

    # excute the code and compare the result
    if lang == "python":
        try:
            # user's code need complete define, include name of function,etc: def add(a, b): return a+b
            exec(user_code)
            # define user's function,and input the data
            result = eval(f"{function_name}(*input_args)")
            print(f"your result: {result}")
            
            if result == expected_output:
                print("congrats ,you pass the test!")
            else:
                print("you didn't pass the test")
        except Exception as e:
            print("you didn't pass the test")
    elif lang == "c":
        try:
            # create temp
            with open('/tmp/temp.c', 'w') as f:
                f.write('#include <stdio.h>\n')  # add header
                f.write(user_code)
            # compile
            subprocess.check_output('gcc /tmp/temp.c -o /tmp/temp.out', shell=True)
            # run c and compare the input
            result = subprocess.check_output('/tmp/temp.out', shell=True).decode('utf-8')  # decode bytes to string
            print(f"your result: {result}")
            
            if result.strip() == expected_output.strip():  # strip to remove leading/trailing whitespace
                print("congrats ,you pass the test!")
            else:
                print("you didn't pass the test")
        except Exception as e:
            print("you didn't pass the test")
        finally:
            # delete temp
            os.remove('/tmp/temp.c')
            if os.path.exists('/tmp/temp.out'):  # delete it only when it exist
                os.remove('/tmp/temp.out')

def communicate_with_tm(s):
    print("testing\n")
    # Create a socket and connect to the TM server

    print("connected\n")
    os.listdir()

    # Send the question and answer to the TM server
    #s.sendall(f"{question}\n{answer}".encode())

    # Receive the response from the TM server
    data = None
    data = s.recv(2)
    # Generate questions
    if data.decode() == "GQ":
        print("generating questions\n")
        questions = generate_questions()
        combined = ''.join(questions)
        s.send(combined.encode())

    # Decode the received data and return it
    return data.decode()

def generate_questions():
    questions = []
    for x in range(6): #generate 4 random questions
        print(x)
        questionID = random.choice(string.ascii_lowercase) #generate random question ID
        print("question id is:\n")
        print(questionID)
        arraylength = len(questions)
        for y in range(arraylength):
            if questions[y] == questionID: #if ID has already been generated successfully, generate a new ID
                print("duplicate question ID")
                x -= 1
                continue
        buffer = grab_questionID(questionID) #grab question ID if it exists
        if buffer == "no": #if index wasn't successfully found
            x -= 1
            continue
        questions.append(buffer)
        questions.append(';')
    return questions

def grab_questionID(questionID):
    with open(PQB, "r", encoding="utf-8") as f:
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

def connect_to_pqb():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((TM_SERVER, PQB_PORT))
        return s

def connect_to_cqb():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((TM_SERVER, CQB_PORT))
        return s
  
def main(argv):
    try:
        arg = getopt.getopt(argv, "pc")
    except:
        print ("select one\n -P for python\n -C for C")
        sys.exit(2)
    if arg == '-p': #python
        s = connect_to_pqb()
        communicate_with_tm(s)
    elif arg == 'c': #c
        s = connect_to_cqb()
        communicate_with_tm(s)
    
if __name__ == "__main__":
    main(sys.argv[1:])