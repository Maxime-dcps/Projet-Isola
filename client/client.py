import pygame
import pygame_gui
import sys
import socket
import errno
import struct
from network import NetworkClient
from protocol_constants import *

# --- PyGame Setup ---
pygame.init()

WIDTH, HEIGHT = 800, 600
SCREEN_SIZE = (WIDTH, HEIGHT)
CAPTION = "Isola Client"

# Initialize display
screen = pygame.display.set_mode(SCREEN_SIZE)
pygame.display.set_caption(CAPTION) # Set the window title

# Initialize the UIManager
MANAGER = pygame_gui.UIManager(SCREEN_SIZE)

CLOCK = pygame.time.Clock()


class IsolaClientApp:
    def __init__(self):
        self.running = True
        self.network = NetworkClient()
        # States: INITIALIZING, CONNECTING, CONNECTED, AWAITING_AUTH, AUTHENTICATED
        self.state = 'CONNECTING'
        self.username = ""
        self.init_login_ui()

    def init_login_ui(self):
        self.login_panel = pygame_gui.elements.UIPanel(
            relative_rect=pygame.Rect((WIDTH // 4, HEIGHT // 4), (400, 250)),
            manager=MANAGER
        )

        # Username Input
        self.username_input = pygame_gui.elements.UITextEntryLine(
            relative_rect=pygame.Rect((130, 20), (200, 30)),
            manager=MANAGER, container=self.login_panel
        )
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 20), (100, 30)),
            text="Username:", manager=MANAGER, container=self.login_panel
        )

        # Password Input
        self.password_input = pygame_gui.elements.UITextEntryLine(
            relative_rect=pygame.Rect((130, 60), (200, 30)),
            manager=MANAGER, container=self.login_panel
        )
        self.password_input.set_text_hidden(True)
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 60), (100, 30)),
            text="Password:", manager=MANAGER, container=self.login_panel
        )

        # Login Button
        self.login_button = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect((130, 100), (200, 40)),
            text="Login / Create Account", manager=MANAGER, container=self.login_panel
        )

        # Status Message Label
        self.status_message = pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 150), (350, 30)),
            text="Status: Initializing...", manager=MANAGER, container=self.login_panel
        )

    def handle_login_button(self, event):
        if event.ui_element == self.login_button:
            username = self.username_input.get_text()
            password = self.password_input.get_text()

            if not password:
                self.status_message.set_text("Invalid input.")
                return
            if self.state != 'CONNECTED':
                self.status_message.set_text("Not connected to the server.")
                return

            self.status_message.set_text("Attempting authentication...")

            # Sends the C_AUTH_CHALLENGE packet
            self.network.send_auth_challenge(username, password)
            self.state = 'AWAITING_AUTH' # Avoid several auth_challenge in case of button spam
            self.username = username

    def process_server_packets(self, packets):
        for command_id, body in packets:

            if command_id == S_AUTH_RESPONSE:
                # 1B Status + 2B wins count + 2B losses + 2B forfeits

                status = body[0]

                if status == S_STATUS_SUCCESS or status == S_STATUS_CREATED:
                    wins, losses, forfeits = struct.unpack(">HHH", body[2:]) # Avoiding the padding (might be a bug)

                    if status == S_STATUS_CREATED:
                        msg = f"Account CREATED! Stats: W:{wins}, L:{losses}"
                    else:
                        msg = f"Login SUCCESS! Stats: W:{wins}, L:{losses}"

                    self.status_message.set_text(f"Status: {msg}")
                    self.state = 'AUTHENTICATED'

                    # TODO: Send to lobby
                    print(f"INFO: Client authenticated as {self.username}. Ready for Matchmaking.")

                elif status == S_STATUS_FAIL:
                    self.status_message.set_text("Status: Authentication FAILED. Try again.")
                    self.state = 'CONNECTED'  # Allows retry

            # TODO: S_WAITING_OPPONENT, S_MATCH_FOUND, etc.

    def run(self):
        # Start the connection attempt
        self.network.connect()

        while self.running:
            # Time delta for UIManager
            time_delta = CLOCK.tick(60) / 1000.0

            # Event loop
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    self.running = False

                if event.type == pygame.USEREVENT:
                    if event.type == pygame_gui.UI_BUTTON_PRESSED:
                        self.handle_login_button(event)

                MANAGER.process_events(event)

            if self.state == 'CONNECTING':
                try:
                    # Test connection
                    self.network.sock.getpeername()
                    self.state = 'CONNECTED'
                    self.status_message.set_text("Status: Connected to Server. Ready to login.")
                except socket.error as e:
                    if e.errno != errno.ENOTCONN:
                        print(f"Connection failed ({e.strerror})")
                        self.running = False
            # Process incoming packet only if CONNECTED or beyond
            else:
                packets = self.network.receive_packet()

                if packets is None:
                    # Server closed the connection
                    print("Connection has been closed.")
                    self.running = False

                if packets:
                    self.process_server_packets(packets)

            # GUI
            MANAGER.update(time_delta)

            MANAGER.draw_ui(screen)

            pygame.display.update()

        # Cleanup
        self.network.sock.close()
        pygame.quit()
        sys.exit()


if __name__ == '__main__':
    # Application startup
    app = IsolaClientApp()
    app.run()