import random
import os
import socket

answered_questions_file = "answered_questions.txt"

# Define your TM server credentials here
TM_SERVER = "192.168.220.118"
TM_PORT = 4125

CQB = "CQuestionBank"

def communicate_with_tm(question, answer):
    # Create a socket and connect to the TM server
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((TM_SERVER, TM_PORT))

        # Send the question and answer to the TM server
        #s.sendall(f"{question}\n{answer}".encode())

        # Receive the response from the TM server
        data = s.recv(1024)

        s.send()

    # Decode the received data and return it
    return data.decode()

def read_question_bank(file_name):
    # Create a socket and connect to the server
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((TM_SERVER, TM_PORT))

        print("connected!\n")

        # Request a specific question file
        #s.sendall(file_name.encode())
        CQB = "CQB"
        s.send(CQB.encode())
        # Receive the file from the server
        data = b''
        while True:
            packet = s.recv(1024)
            if not packet:
                break
            data += packet

    # Write the received data to a file
    with open(file_name, "wb") as f:
        f.write(data)

    with open(file_name, "r", encoding="utf-8") as f:
        lines = f.read().splitlines()
    questions = []
    i = 0
    while i < len(lines):
        if not lines[i].strip():
            i += 1
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
    questions = read_question_bank("Questionbank.txt")
    answered_questions = load_answered_questions()
    questions = [q for q in questions if q["content"] not in answered_questions]
    if not questions:
        print("all question answered")
        return

    random.shuffle(questions)
    question = questions[0]

    print(question["content"])
    if question["type"] == "mcq":
        for i, option in enumerate(question["options"], start=1):
            print(f"{i}. {option}")
        attempts = 3
        while attempts > 0:
            answer = input()
            if answer.isdigit() and 1 <= int(answer) <= len(question["options"]):
                selected_option = question["options"][int(answer)-1]
                if selected_option == question["answer"]:
                    print("right!")
                    break
                else:
                    response = communicate_with_tm(question["content"], answer)
                    print(response)
                    attempts -= 1
                    if attempts > 0:
                        print(f"wrong,{attempts}chances left")
                    else:
                        print("wrong,no more chance")
            else:
                print("invalid input")
    elif question["type"] == "programming challenge":
        print("this is a programming challenge")
        user_answer = input()
        attempts = 3
        while attempts > 0:
            if user_answer.strip() == question["answer"]:
                print("right!")
                break
            else:
                attempts -= 1
                if attempts > 0:
                    print(f"wrong,{attempts}chances left")
                    user_answer = input()
                else:
                    print("wrong,no more chance")
    save_answered_question(question["content"])


if __name__ == "__main__":
    main()