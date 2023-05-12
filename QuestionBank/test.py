import random
import subprocess
import os

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
