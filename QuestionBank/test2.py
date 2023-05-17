import socket

def send_data(s, data):
    # Convert the data to bytes
    data_bytes = data.encode()

    # Get the length of the data and convert it to network byte order
    length = len(data_bytes)
    length_net = socket.htonl(length).to_bytes(4, 'big')  # Convert host byte order to network byte order

    # Send the length of the data
    s.send(length_net)

    # Send the data
    s.send(data_bytes)

client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect(("192.168.0.5", 4126))

print("Connected to the server.")

# Send the "AN" command
client_socket.send(b"AN")

print("Sent AN command. Sending answer and question ID...")

# Send the answer
answer = "return a+b"
send_data(client_socket, answer)

# Convert the question ID to network byte order
question_id = "11"  # Replace with your actual question ID
question_id_net = socket.htonl(int(question_id)).to_bytes(4, 'big')

# Send the question ID
client_socket.send(question_id_net)

print("Answer and question ID sent. Waiting for response...")

# Receive and print the response
response = client_socket.recv(1).decode()
print(f"Received response: {response}")

# Remember to close the connection when you're done
client_socket.close()
