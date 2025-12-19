import socket
import struct
import errno
import hashlib
from protocol_constants import *


class NetworkClient:
    def __init__(self, host='127.0.0.1', port=SERVER_PORT):
        self.host = host
        self.port = port
        self.sock = None
        self.recv_buffer = b''  # Packet buffer
        print(f"INFO: Initializing network client for {host}:{port}")

    def connect(self):
        # Start connection with the server
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Create a new socket
            self.sock.setblocking(False) # Config the socket as non-blocking
            self.sock.connect_ex((self.host, self.port)) # Try to connect to server in TCP
            return True
        except Exception as e:
            print(f"ERROR: Failed to initialize socket: {e}")
            return False

    def send_packet(self, command_id: int, body: bytes = b''):
        # Builds and sends a packet

        header = struct.pack(">B x H", command_id, len(body)) # >=Big endian, B=uint8, x=reserved, H=uint16
        packet = header + body

        try:
            self.sock.sendall(packet) # Sends the full packet
            return True
        except socket.error as e:
            print(f"ERROR: Failed to send packet: {e}")
            return False

    def receive_packet(self):
        # Reads data and extracts packets

        try:
            # Tries to read data.
            data = self.sock.recv(4096) # Raises an exception if nothing available
            if not data:
                # The connection has been closed
                return None
            self.recv_buffer += data
        except socket.error as e:
            # Handles the exception if not EWOULDBLOCK or EAGAIN
            if e.errno not in (errno.EWOULDBLOCK, errno.EAGAIN):
                print(f"ERROR: Socket read error: {e}")
                return None

        packets = []
        # As long as there's enough data in the buffer for at least a Header
        while len(self.recv_buffer) >= HEADER_SIZE:

            header_data = self.recv_buffer[:HEADER_SIZE] # Copy the HEADER_SIZE first bytes

            command_id, length = struct.unpack_from(">B x H", header_data, 0) # Unpack the ID and the body's length

            total_packet_size = HEADER_SIZE + length

            # If the packet is complete
            if len(self.recv_buffer) >= total_packet_size:

                packet_body = self.recv_buffer[HEADER_SIZE:total_packet_size] # Extract the packet without the header
                packets.append((command_id, packet_body))

                self.recv_buffer = self.recv_buffer[total_packet_size:] # Shift the remaining data in the buffer to avoid deleting the next packet

        return packets

    def send_auth_challenge(self, username, password):
        # Sends C_AUTH_CHALLENGE
        username_bytes = username.encode('utf-8')[:MAX_USERNAME_LEN] # Might change for ascii

        username_padded = username_bytes.ljust(MAX_USERNAME_LEN, b'\x00') # Adds bits to match the size

        password_hash = hashlib.sha256(password.encode('utf-8')).digest() # Hash the password

        body = struct.pack(f'{MAX_USERNAME_LEN}s 32s', username_padded, password_hash)

        return self.send_packet(C_AUTH_CHALLENGE, body)

    def send_change_password(self, old_password, new_password):
        # Sends C_CHANGE_PASSWORD with old and new password hashes
        old_hash = hashlib.sha256(old_password.encode('utf-8')).digest()
        new_hash = hashlib.sha256(new_password.encode('utf-8')).digest()
        
        body = struct.pack('32s 32s', old_hash, new_hash)
        
        return self.send_packet(C_CHANGE_PASSWORD, body)