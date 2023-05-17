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
client_socket.connect(("192.168.0.5", 4127))

print("Connected to the server.")

# Send the "IQ" command
client_socket.send(b"IQ")

print("Sent IQ command. Sending question ID...")

# Convert the question ID to network byte order
question_id = "11"  # Replace with your actual question ID
question_id_net = socket.htonl(int(question_id)).to_bytes(4, 'big')

# Send the question ID
client_socket.send(question_id_net)

print("question ID sent. Waiting for response...")

# Receive and print the response
response = client_socket.recv(1024).decode()
print(f"Received response: {response}")

# Remember to close the connection when you're done
client_socket.close()
