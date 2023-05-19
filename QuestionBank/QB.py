import random
import os
import socket
import string
import sys, getopt
import select

answered_questions_file = "answered_questions.txt"

# Define your TM server credentials here
TM_SERVER = "45.248.79.109"
PQB_PORT = 4126
CQB_PORT = 4127
PQB = "PythonQuestionBank"
CQB = "CQuestionBank"
PQBCount = 6
CQBCount = 4

def communicate_with_tm(s, version):
    # Receive the response from the TM server
    while True:
        print("waiting for TM")
        data = s.recv(2)
        print(data)
        pid = os.fork()
        if pid < 0:
            print("Fork failed")
            break
        if pid > 0:
            if not data:
                break
        else:
            if not data:
                print("Connection closed by server.")
                break
            decoded_data = data.decode()
            if decoded_data == "GQ": #Generate questions
                print("generating questions\n")
                questions = generate_questions(version)
                combined = ''.join(questions)
                print(combined)
                s.send(combined.encode())
                exit(0)
            elif decoded_data == "AN": # Receive Answer and return if correct
                    # Receive the size of the answer
                answer_size = int(s.recv(4).decode()) #ints are only 4 bytes
                received_bytes = 0
                answer = ''
                while received_bytes < answer_size:
                    chunk = s.recv(answer_size - received_bytes)
                    received_bytes += len(chunk)
                    answer += chunk.decode()
            elif decoded_data == "PQ": #return question info from questionID
                exit(0)
            elif decoded_data == "IQ": #return answer from questionID
                exit(0)
            elif decoded_data == "ex":
                print("Exit command received. Closing connection.")
                break
            else:
                print("Invalid command received. Closing connection.")
                break
            
    # Decode the received data and return it

def generate_questions(count):
    questions = []
    generated_ids = set()
    while len(questions) < count * 2: #generate counted random questions
        questionID = random.choice(string.ascii_lowercase) #generate random question ID
        print("question id is:\n")
        print(questionID)
        if questionID in generated_ids: #if ID has already been generated successfully, generate a new ID
            continue
        buffer = grab_questionID(questionID) #grab question ID if it exists
        if buffer == "no": #if index wasn't successfully found
            continue
        generated_ids.add(questionID)
        questions.append(buffer)
        questions.append(';')
    return questions

def grab_questionID(questionID):
    with open(CQB, "r", encoding="utf-8") as f:
        for line in f:
            buff = line.strip()
            if buff == questionID:
                print(questionID)
                print("Found question id")
                print(buff)
                return buff
    return "no"

def connect_to_tm(port):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((TM_SERVER, port))
        print("connected")
        return s
    except socket.error as e:
        print(f"Unable to connect to the server: {e}")
        sys.exit(1)


def parse_question(question_text):
    lines = question_text.strip().split('\n')
    question_id = lines[0]
    if len(lines) == 5:  # MCQ question
        question = lines[1]
        options = lines[2:5]
        correct_answer = lines[5]
        solution_code = None
    else:  # Programming challenge
        question = lines[1]
        inputs, correct_output, solution_code = ast.literal_eval(lines[2])
        options = None
        correct_answer = (inputs, correct_output)
    return question_id, question, options, correct_answer, solution_code

def run_code(language, code, inputs):
    if language.lower() == 'c':
        with open('temp.c', 'w') as file:
            file.write(code)
        subprocess.call(['gcc', 'temp.c', '-o', 'temp.out'])
        output = subprocess.check_output(['./temp.out', json.dumps(inputs)]).decode()
        return output.strip()
    elif language.lower() == 'python':
        output = subprocess.check_output(['python', '-c', f"{code}\nprint(solution(*{inputs}))"]).decode()
        return output.strip()
    else:
        raise ValueError(f"Unsupported language: {language}")

def check_answer_cqb(question_text, user_answer):
    question_id, question, options, correct_answer, solution_code = parse_question(question_text)
    if options is not None:  # MCQ question
        return user_answer.lower() == correct_answer.lower()
    else:  # Programming challenge
        inputs, correct_output = correct_answer
        user_output = run_code('c', user_answer, inputs)
        return user_output == correct_output.strip()

def check_answer_pqb(question_text, user_answer):
    question_id, question, options, correct_answer, solution_code = parse_question(question_text)
    if options is not None:  # MCQ question
        return user_answer.lower() == correct_answer.lower()
    else:  # Programming challenge
        inputs, correct_output = correct_answer
        user_output = run_code('python', user_answer, inputs)
        return user_output == correct_output.strip()

  
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
            communicate_with_tm(s, PQBCount)
        elif opt == '-c': #c
            print("conencting to c")
            s = connect_to_tm(CQB_PORT)
            communicate_with_tm(s, CQBCount) 
    
if __name__ == "__main__":
    main()