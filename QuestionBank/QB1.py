import random
import os
import socket
import string
import sys, getopt
import select
import struct
import subprocess
import ast
import tempfile

answered_questions_file = "answered_questions.txt"

# Define your TM server credentials here
TM_SERVER = "localhost"
PQB_PORT = 4126
CQB_PORT = 4127
PQB = "PythonQuestionBank"
CQB = "CQuestionBank"
PQBCount = 6
CQBCount = 4

def start_server(version, QBS, TM_socket):
    #server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    #server_socket.bind(("192.168.0.5", 4127))
    #server_socket.listen(1)

    while True:  # Infinite loop to accept new connections
        # Now we handle the client connection in a separate loop
        print("waiting for TM")
        data = TM_socket.recv(2)
        if not data:
            print("No data received from client. Closing connection.")
            break
        print("recieved data")
        print(data.decode())

        if data.decode() == "GQ":
            RQB = get_random_questions(QBS, version)  # Create a new RQB for each client
            generate_questions(TM_socket, RQB)
        elif data.decode() == "AN": #answer question
            receive_answer(TM_socket, QBS)
        elif data.decode() == "IQ": #incorrect question
            send_answer(TM_socket, QBS)
        elif data.decode() == "PQ": #return question text
            recv_id_and_return_question_info(TM_socket, QBS)
        else:
            print("Invalid command received. Closing connection.")
            break

    # The server socket is never closed in this example, because the server runs forever

def generate_questions(s, question_dict):
        # Join the question IDs with a semicolon
        question_ids = ";".join(str(id) for id in question_dict.keys())
        print(question_ids)
        question_ids += ";"
        # Encode the string to bytes (required for sending via socket)
        question_ids = question_ids.encode()
        # Send the question IDs
        send_data(s, question_ids)

def send_data(s, data):
        # Get the data size
        data_size = len(data)
        # Send the data size
        s.sendall(struct.pack('!i', data_size))
        # Send the data
        s.sendall(data)

def receive_answer(s,QBS):
    # Receive the length of the answer
    length_net = s.recv(4)
    length = socket.ntohl(int.from_bytes(length_net, 'big'))  # Convert network byte order to host byte order

    # Receive the answer
    answer = s.recv(length).decode()

    # Receive the question ID
    question_id_net = s.recv(4)
    question_id = socket.ntohl(int.from_bytes(question_id_net, 'big'))  # Convert network byte order to host byte order

    # Process the answer and question ID...
    print(f"Question ID: {question_id}")
    question = QBS[str(question_id)]
    print(f"Question: {question['question']}")
    print(f"Correct answer: {question['answer']}")
    print(f"User answer: {answer}")
    # If the answer is correct, send "Y". Otherwise, send "N".
    result = check_answer(QBS, question_id,answer)  # You need to implement this function
    print(f"Sending response: {result}")

    # Calculate the length of the response
    result_length = len(result)
    result_length_net = socket.htonl(result_length).to_bytes(4, 'big')

    # Send the length of the result
    s.send(result_length_net)

    # Send the result
    s.send(result.encode())

def recv_id_and_return_question_info(s, question_dict):
    # Receive 4 bytes of data (the size of an integer in network byte order)
    id_bytes = s.recv(4)

    # Check if we received 4 bytes
    if len(id_bytes) < 4:
        print("Did not receive 4 bytes for ID. Closing connection.")
        return

    # Unpack the bytes to get the integer ID
    id = struct.unpack('!i', id_bytes)[0]

    # Return the question info for this ID
    return_question_info(s, question_dict, id)

def return_question_info(s, question_dict, id):
    # Check if the ID is valid
    if str(id) not in question_dict:
        print(f"Invalid question ID: {id}")
        return

    # Get the question and options
    question_data = question_dict[str(id)]
    question = question_data['question']
    options = question_data['options']
    type = question_data['type']
    # If options is None, set it to an empty string
    if options is None:
        options = ""

    # Debugging output
    print(f"Question data for ID {id}: {question_data}")
    print(f"Options data type: {type(options)}")

    # If options is a list, convert it to a string
    if isinstance(options, list):
        options = ', '.join(options)

    # Convert question, options, and id to bytes
    question_bytes = question.encode()
    options_bytes = options.encode()
    id_bytes = struct.pack('!i', id)  # Convert id to network byte order

    # Send length of question, question, length of options, options, and id
    s.sendall(struct.pack('!i', len(question_bytes)))  # Send length of question
    s.sendall(question_bytes)  # Send question
    s.sendall(struct.pack('!i', len(options_bytes)))  # Send length of options
    s.sendall(options_bytes)  # Send options
    s.sendall(struct.pack('!i', len(type))) #send length of type
    s.sendall(type) #send type
    s.sendall(id_bytes)  # Send id
    print("sent all data")


def send_answer(s, question_dict):
    # Receive QuestionID
    question_id_net = s.recv(4)
    question_id = socket.ntohl(int.from_bytes(question_id_net, 'big'))  # Convert network byte order to host byte order
    
    # Try to convert the received ID to integer. If it fails, it's probably not a valid ID.
    try:
        question_id = int(question_id)
    except ValueError:
        print(f"Received an invalid QuestionID: {question_id}")
        return

    # Look up the question data for this id
    question_data = question_dict.get(str(question_id))

    if question_data is None:
        print(f"No data found for QuestionID {question_id}")
        return

    # Extract the answer
    answer = question_data['answer']

    # Send the answer back
    s.send(answer.encode())

def connect_to_tm(port):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((TM_SERVER, port))
        print("connected")
        return s
    except socket.error as e:
        print(f"Unable to connect to the server: {e}")
        sys.exit(1)

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
            answer = lines[i+6]
            i += 7
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

    random_questions = {qid: question_dict[qid] for qid in random_question_ids}

    return random_questions

def check_answer(question_dict, id, user_answer):
    question = question_dict[str(id)]
    inputs = ast.literal_eval(question['input'])

    if question['type'] == 'programchallenge':
        if question['category'] == 'python':
            try:
                # Write user's answer to a temp file
                with tempfile.NamedTemporaryFile(suffix=".py", delete=False) as tmpfile:
                    python_code_file = tmpfile.name
                    tmpfile.write(user_answer.encode())

                # Prepare the command to run the Python code with the provided inputs
                cmd = f"python3 {python_code_file} {' '.join(inputs)}"
                
                # Run the Python code, and get the output
                result = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                if result.returncode != 0:
                    return result.stdout.decode() + result.stderr.decode()
                user_output = result.stdout.decode().strip()
            except Exception as e:
                return str(e)

        elif question['category'] == 'C':
            try:
                # Write user's answer to a temp file
                with tempfile.NamedTemporaryFile(suffix=".c", delete=False) as tmpfile:
                    c_code_file = tmpfile.name
                    tmpfile.write(user_answer.encode())

                # Prepare the commands to compile and run the C code with the provided inputs
                commands = [
                    f"gcc -o {c_code_file[:-2]} {c_code_file}",
                    f"{c_code_file[:-2]} {' '.join(inputs)}"
                ]

                # Compile and run the C code, and get the output
                for cmd in commands:
                    result = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    if result.returncode != 0:
                        return f"Program returned with non-zero exit status {result.returncode}. Error output: {result.stderr.decode()}"
                user_output = result.stdout.decode().strip()
            except Exception as e:
                return str(e)
        else:
            return "Invalid programming language."

        expected_output = question['expected_output'].strip('"')
        
        if str(user_output) != str(expected_output):
            return f"Expected output '{expected_output}' but got '{user_output}'."
        else:
            return "Y"

    else:
        return "Invalid question type."
  
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
            # communicate_with_tm(s, PQBCount, QBS)
            start_server(PQBCount,QBS,s)
        elif opt == '-c': #c
            print("conencting to c")
            QBS=parse_data(CQB)
            s = connect_to_tm(CQB_PORT)
            # communicate_with_tm(s, CQBCount, QBS)
            start_server(CQBCount,QBS,s) 
    
if __name__ == "__main__":
    main()
