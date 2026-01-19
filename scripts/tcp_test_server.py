import socket
import sys

# Configuration matches onvm/onvm_mgr/main.c
HOST = '127.0.0.1'
PORT = 1234

def start_server():
    print(f"Starting TCP server on {HOST}:{PORT}...")
    
    try:
        # Create a TCP/IP socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        
        # Allow reuse of the address to avoid "Address already in use" errors on restart
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # Bind the socket to the address
        sock.bind((HOST, PORT))
        
        # Listen for incoming connections
        sock.listen(1)
        print("Server is listening. Please start onvm_mgr now.")
        
        while True:
            print("Waiting for a connection...")
            connection, client_address = sock.accept()
            try:
                print(f"Connection established from: {client_address}")
                
                while True:
                    data = connection.recv(1024)
                    if not data:
                        print(f"Connection closed by {client_address}")
                        break
                    
                    # Decode and print the message
                    # Using 'replace' to handle potentially non-printable characters like null terminators gracefully
                    try:
                        message = data.decode('utf-8', errors='replace')
                        print(f"Received: {repr(message)}") 
                    except Exception as e:
                        print(f"Received raw data: {data} (Error decoding: {e})")
                        
            finally:
                connection.close()
                
    except KeyboardInterrupt:
        print("\nServer stopping by user request.")
    except Exception as e:
        print(f"\nServer error: {e}")
    finally:
        try:
            sock.close()
        except:
            pass

if __name__ == "__main__":
    start_server()
