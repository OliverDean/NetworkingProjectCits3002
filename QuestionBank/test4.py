import socket
import struct

def send_data(s, data):
    # Convert the data to bytes
    data_bytes = data.encode()

    # Send the data
    s.send(data_bytes)

client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect(("192.168.0.5", 4126))

print("Connected to the server.")

# Send the "PQ" command
send_data(client_socket, "PQ")

print("Sent PQ command. Sending question ID...")

# Convert the question ID to network byte order
question_id = 8  # Replace with your actual question ID
question_id_net = struct.pack('!i', question_id)

# Send the question ID
client_socket.send(question_id_net)

print("question ID sent. Waiting for response...")

# Receive the length of question
question_length_net = client_socket.recv(4)
question_length = struct.unpack('!i', question_length_net)[0]

# Receive and print the question
question = client_socket.recv(question_length).decode()
print(f"Received question: {question}")

# Receive the length of options
options_length_net = client_socket.recv(4)
options_length = struct.unpack('!i', options_length_net)[0]

# Receive and print the options
options = client_socket.recv(options_length).decode()
print(f"Received options: {options}")

# Remember to close the connection when you're done
client_socket.close()
