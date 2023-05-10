
import http.server

import argparse


# Define the command-line arguments
parser = argparse.ArgumentParser(description="HTTP server")
parser.add_argument("-p", "--port", type=int, default=8000, help="Port number to listen on")
args = parser.parse_args()

