import random
import subprocess
import tempfile

PQB = "PythonQuestionBank"
CQB = "CQuestionBank"


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

# def get_random_questions(question_dict, num_questions):
#     question_ids = list(question_dict.keys())
#     random_question_ids = random.sample(question_ids, num_questions)

#     random_questions = {}
#     for new_id, old_id in enumerate(random_question_ids, start=1):
#         random_questions[new_id] = question_dict[old_id]

#     return random_questions

        
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
    question_dict = parse_data(CQB)
    random_questions = get_random_questions(question_dict, num_questions=2)
    print(check_answer(question_dict, "6", 'return a+b;'))

if __name__ == "__main__":
    main()
