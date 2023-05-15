import random
import os
import socket
import string
import sys, getopt
import select

answered_questions_file = "answered_questions.txt"

# Define your TM server credentials here
TM_SERVER = "192.168.243.118"
PQB_PORT = 4126
CQB_PORT = 4127
PQB = "PythonQuestionBank"
CQB = "CQuestionBank"
PQBCount = 6
CQBCount = 4

def communicate_with_tm(s, version):
    RQB=get_random_questions(QBS,version)
    # Functions to execute for each command
    commands = {
        "GQ": generate_questions(s,RQB),
        "AN": receive_answer(s),
        "PQ": recv_id_and_return_question_info(s, RQB),
        "IQ": recv_id_and_return_question_info(s, RQB),  # assume PQ IQ all receive ID and need return related data
    }

    while True:
        print("waiting for TM")
        data = s.recv(2)

        if not data:
            print("Connection closed by server.")
            break

        decoded_data = data.decode()

        if decoded_data in commands:
            function_to_execute = commands[decoded_data]
            # Assuming all functions take the same arguments
            function_to_execute(s, version)
        else:
            print("Invalid command received. Closing connection.")
            break


def receive_answer(s):
    # Receive the length of the answer
    length_net = s.recv(4)
    length = socket.ntohl(int.from_bytes(length_net, 'big'))  # Convert network byte order to host byte order

    # Receive the answer
    answer = s.recv(length).decode()

    # Receive the question ID
    question_id_net = s.recv(4)
    question_id = socket.ntohl(int.from_bytes(question_id_net, 'big'))  # Convert network byte order to host byte order

    # Process the answer and question ID...
    # If the answer is correct, send "Y". Otherwise, send "N".
    is_answer_correct = check_answer(answer, question_id)  # You need to implement this function
    s.send(('Y' if is_answer_correct else 'N').encode())

def generate_questions(s, question_dict):
    for id, question_data in question_dict.items():
        question = question_data['question']
        options = question_data['options']

        # Convert question, options, and id to bytes
        question_bytes = question.encode()
        options_bytes = options.encode()
        id_bytes = struct.pack('!i', id)  # Convert id to network byte order

        # Send length of question, question, length of options, options, and id
        s.sendall(struct.pack('!i', len(question_bytes)))  # Send length of question
        s.sendall(question_bytes)  # Send question
        s.sendall(struct.pack('!i', len(options_bytes)))  # Send length of options
        s.sendall(options_bytes)  # Send options
        s.sendall(id_bytes)  # Send id

def connect_to_tm(port):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((TM_SERVER, port))
        print("connected")
        return s
    except socket.error as e:
        print(f"Unable to connect to the server: {e}")
        sys.exit(1)

def recv_id_and_return_question_info(s, question_dict):
    # Receive 4 bytes of data (the size of an integer in network byte order)
    id_bytes = s.recv(4)

    # Unpack the bytes to get the integer ID
    id = struct.unpack('!i', id_bytes)[0]

    # Return the question info for this ID
    return_question_info(s, question_dict, id)

def return_question_info(s, question_dict, id):
    # Check if the ID is valid
    if id not in question_dict:
        print(f"Invalid question ID: {id}")
        return

    # Get the question and options
    question_data = question_dict[id]
    question = question_data['question']
    options = question_data['options']

    # Convert question, options, and id to bytes
    question_bytes = question.encode()
    options_bytes = options.encode()
    id_bytes = struct.pack('!i', id)  # Convert id to network byte order

    # Send length of question, question, length of options, options, and id
    s.sendall(struct.pack('!i', len(question_bytes)))  # Send length of question
    s.sendall(question_bytes)  # Send question
    s.sendall(struct.pack('!i', len(options_bytes)))  # Send length of options
    s.sendall(options_bytes)  # Send options
    s.sendall(id_bytes)  # Send id



def parse_data(QB):
    with open(QB, "r") as file:
        lines = file.read().split('\n')

    question_dict = {}
    i = 0
    while i < len(lines):
        id = lines[i]
        type = lines[i+1]
        category = None
        question_text = None
        options = None
        answer = None
        input_data = None
        expected_output = None

        if type == 'mcq':
            question_text = lines[i+2]
            options = lines[i+3:i+7]
            answer = lines[i+7]
            i += 8
        elif type == 'programchallenge':
            category = lines[i+2]
            question_text = lines[i+3]
            input_data = lines[i+4].split('Input:')[1].strip()
            expected_output = lines[i+5].split('Output:')[1].strip()
            i += 6
        else:
            i += 1
            continue

        question_dict[id] = {
            'type': type,
            'category': category,
            'question': question_text,
            'options': options,
            'answer': answer,
            'input': input_data,
            'expected_output': expected_output
        }

    return question_dict


def get_random_questions(question_dict, num_questions):
    question_ids = list(question_dict.keys())
    random_question_ids = random.sample(question_ids, num_questions)

    random_questions = {}
    for new_id, old_id in enumerate(random_question_ids, start=1):
        random_questions[new_id] = question_dict[old_id]

    return random_questions

        
def check_answer(question_dict, id, user_answer):
    question = question_dict[id]

    if question['type'] == 'mcq':
        return user_answer.lower() == question['answer'].lower()
    elif question['type'] == 'programchallenge':
        # Remove parentheses and split the input string into two parts
        inputs = question['input'].strip('()').split(',')
        # Check if there are enough inputs
        if len(inputs) < 2:
            return False
        # Remove the double quotes and any spaces around each input value, then convert them to integers
        a, b = map(int, (i.strip('" ').strip() for i in inputs))
        expected_output = question['expected_output'].strip('"')

        if question['category'] == 'python':
            # Create a function definition with the user's answer as the body
            user_answer_code = f"""
def user_function(a, b):
    {user_answer}
"""
            try:
                # Execute the function definition
                exec(user_answer_code, globals())
                # Call the user's function and get the output
                user_output = user_function(a, b)
            except:
                return False
        elif question['category'] == 'C':
            c_program_template = f"""
#include <stdio.h>
#include <stdlib.h>

int user_function(int a, int b);

int main(int argc, char *argv[]) {{
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    printf("%d", user_function(a, b));
    return 0;
}}

int user_function(int a, int b) {{
    {user_answer}
}}
"""
            # Create a temporary file for the C code
            with tempfile.NamedTemporaryFile(suffix=".c", delete=False) as tmpfile:
                c_code_file = tmpfile.name
                tmpfile.write(c_program_template.encode())

            # Create the command to compile and run the C code
            commands = [
                f"gcc -o {c_code_file[:-2]} {c_code_file}",
                f"{c_code_file[:-2]} {a} {b}"
            ]
            
            # Compile and run the C code, and get the output
            try:
                for cmd in commands:
                    result = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    if result.returncode != 0:
                        return False
                user_output = result.stdout.decode().strip()
            except:
                return False
        else:
            return False

        # Convert expected_output to correct type based on its own value
        if expected_output.isdigit():
            expected_output = int(expected_output)
        elif expected_output.lower() in ['true', 'false']:
            expected_output = expected_output.lower() == 'true'
        
        return str(user_output) == str(expected_output)


  
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
            QBS=parse_data(PQB)
            s = connect_to_tm(PQB_PORT)
            communicate_with_tm(s, PQBCount)
        elif opt == '-c': #c
            print("conencting to c")
            QBS=parse_data(CQB)
            s = connect_to_tm(CQB_PORT)
            communicate_with_tm(s, CQBCount) 
    
if __name__ == "__main__":
    main()