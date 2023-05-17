import socket

def send_data(s, data):
    # Convert the data to bytes
    data_bytes = data.encode()

    # Send the data
    s.send(data_bytes)

client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect(("192.168.0.5", 4126))

print("Connected to the server.")

send_data(client_socket, "GQ")

print("Sent GQ command. Waiting for response...")

# Receive and print the question IDs
question_ids = client_socket.recv(1024).decode()
print(f"Received question IDs: {question_ids}")

# Now you can send "IQ" commands for each question ID, receive the question info, etc.

# Remember to close the connection when you're done
client_socket.close()
