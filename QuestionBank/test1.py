import socket

def recv_data(s):
    # Receive the length of the data
    length_net = s.recv(4)
    length = socket.ntohl(int.from_bytes(length_net, 'big'))  # Convert network byte order to host byte order

    # Receive the data
    data = s.recv(length).decode()

    return data

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind(("192.168.0.5", 4126))
server_socket.listen(1)

print("Waiting for a connection...")

client_socket, client_address = server_socket.accept()

print(f"Accepted connection from {client_address}.")

client_socket.send(b"GQ")

print("Sent GQ command. Waiting for response...")

# Receive and print the question IDs
question_ids = recv_data(client_socket)
print(f"Received question IDs: {question_ids}")

# Now you can send "IQ" commands for each question ID, receive the question info, etc.

# Remember to close the connection when you're done
client_socket.close()
server_socket.close()
