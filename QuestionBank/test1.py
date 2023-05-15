import random

def parse_data():
    with open("CQuestionBank", "r") as file:
        lines = file.read().split('\n')

    question_dict = {}
    i = 0
    while i < len(lines):
        id = lines[i]
        type = 'mcq' if 'mcq' in lines[i+1] else 'programchallenge'
        question_text = lines[i+2]
        if type == 'mcq':
            options = lines[i+3:i+7]
            answer = lines[i+7]
            input_data = None
            expected_output = None
            i += 9
        else:
            options = None
            answer = None
            input_data = lines[i+3].split('Input:')[1].strip()
            expected_output = lines[i+4].split('Output:')[1].strip()
            i += 6

        question_dict[id] = {
            'type': type,
            'question': question_text,
            'options': options,
            'answer': answer,
            'input': input_data,
            'expected_output': expected_output
        }

    return question_dict

def get_random_questions(question_dict, num_questions=10):
    question_ids = list(question_dict.keys())
    random_question_ids = random.sample(question_ids, num_questions)

    random_questions = {id: question_dict[id] for id in random_question_ids}

    return random_questions

def main():
    question_dict = parse_data()
    random_questions = get_random_questions(question_dict, num_questions=10)

    for id, question in random_questions.items():
        print(f"Question ID: {id}")
        print(f"Type: {question['type']}")
        print(f"Question: {question['question']}")
        if question['type'] == 'mcq':
            print(f"Options: {question['options']}")
            print(f"Answer: {question['answer']}")
        else:
            print(f"Input: {question['input']}")
            print(f"Expected output: {question['expected_output']}")
        print()

if __name__ == "__main__":
    main()
