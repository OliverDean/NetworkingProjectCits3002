import random
import os
import socket
import string
import time

answered_questions_file = "answered_questions.txt"

# Define your TM server credentials here
TM_SERVER = "192.168.0.195"
CQB_PORT = 4127

CQB = "CQuestionBank.txt"

def communicate_with_tm():
    print("testing\n")
    # Create a socket and connect to the TM server
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((TM_SERVER, CQB_PORT))

        print("connected\n")
        os.listdir()

        # Send the question and answer to the TM server
        #s.sendall(f"{question}\n{answer}".encode())

        # Receive the response from the TM server
        data = s.recv(2)
        print("recieved data ")
        print(data.decode())
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
            

def read_question_bank(questionID):
    # Create a socket and connect to the server
    #with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        #s.connect((TM_SERVER, CQB_PORT))

        #print("connected!\n")
        # Request a specific question file
        #s.sendall(file_name.encode())
        
        # Receive the file from the server
        #data = b''
        #while True:
        #    packet = s.recv(1024)
        #    if not packet:
        #        break
        #    data += packet

    # Write the received data to a file
    #with open(file_name, "wb") as f:
    #    f.write(data)

    with open(CQB, "r", encoding="utf-8") as f:
        lines = f.read().splitlines()
    questions = []
    i = 0
    while i < len(lines):
        if not lines[i].strip():
            i += 1
            continue
        if lines[i].strip() != questionID.decode():
            continue
        question_type = lines[i].strip()
        i += 1
        content = lines[i].strip()
        i += 1
        if question_type == "mcq":
            options = []
            while i < len(lines) and lines[i].strip() and not lines[i].strip().isdigit():
                options.append(lines[i])
                i += 1
            answer = options[-1]
            question = {"type": "mcq", "content": content, "options": options[:-1], "answer": answer}
            questions.append(question)
        elif question_type == "programming challenge":
            answer = lines[i].strip()
            i += 1
            question = {"type": "programming challenge", "content": content, "answer": answer}
            questions.append(question)
        else:
            print(f"unrecognized question type {question_type}")
            continue
    return questions


def execute_test_case(user_func, test_case):
    inputs, expected_output = test_case
    user_output = user_func(*inputs)
    return user_output == expected_output


def load_answered_questions():
    if os.path.exists(answered_questions_file):
        with open(answered_questions_file, "r", encoding="utf-8") as f:
            return set(f.read().splitlines())
    else:
        return set()


def save_answered_question(question_content):
    with open(answered_questions_file, "a", encoding="utf-8") as f:
        f.write(question_content + "\n")


def main():
    communicate_with_tm()
    


if __name__ == "__main__":
    #while 1:
    main()