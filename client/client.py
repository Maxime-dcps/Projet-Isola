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

    def init_lobby_ui(self):
        self.lobby_panel = pygame_gui.elements.UIPanel(
            relative_rect=pygame.Rect((WIDTH // 4, HEIGHT // 4), (400, 300)),
            manager=MANAGER,
            visible=0
        )

        # Welcome message
        self.welcome_label = pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 10), (350, 40)),
            text=f"Welcome, {self.username} !",
            manager=MANAGER, container=self.lobby_panel
        )

        # Simple data label
        self.stats_label = pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 50), (350, 40)),
            text="Stats: Loading...",
            manager=MANAGER, container=self.lobby_panel
        )

        # Play Button
        self.play_button = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect((100, 110), (200, 50)),
            text="FIND MATCH",
            manager=MANAGER, container=self.lobby_panel
        )

        # Lobby Status
        self.lobby_status = pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 180), (350, 40)),
            text="Status: Ready to play",
            manager=MANAGER, container=self.lobby_panel
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

                    self.state = 'AUTHENTICATED'

                    # UI Transition
                    self.login_panel.hide()
                    self.init_lobby_ui()
                    self.lobby_panel.show()

                    # Update Labels with actual data
                    self.welcome_label.set_text(f"Welcome, {self.username}!")
                    self.stats_label.set_text(f"Wins: {wins} | Losses: {losses} | Forfeits: {forfeits}")

                    if status == S_STATUS_CREATED:
                        self.lobby_status.set_text("Status: Account created successfully!")
                    else:
                        self.lobby_status.set_text("Status: Logged in.")

                elif status == S_STATUS_FAIL:
                    self.status_message.set_text("Status: Authentication FAILED. Try again.")
                    self.state = 'CONNECTED'  # Allows retry
            elif command_id == S_WAITING_OPPONENT:
                self.lobby_status.set_text("Status: Searching for opponent...")
                self.play_button.disable()

            elif command_id == S_MATCH_FOUND:
                self.lobby_status.set_text("Status: Match Found! Starting...")
                # TODO: Init to Game Scene
                print("DEBUG: Matchmaking successful, preparing board.")

            elif command_id == S_GAME_STATE:
                # 48B Board + 1B Turn flag = 49 bytes
                board_data = body[:48]
                my_turn = body[48]

                self.init_game_ui(board_data, my_turn)

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

                if event.type == pygame_gui.UI_BUTTON_PRESSED:
                    if event.ui_element == self.login_button:
                        self.handle_login_button(event)
                    elif hasattr(self, 'play_button') and event.ui_element == self.play_button:
                        self.network.send_packet(C_PLAY_REQUEST)

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

    def send_move(self, row, col):
        encoded_val = (row << 5) | (col << 2)

        # Pack into 1 byte (uint8_t)
        body = struct.pack("B", encoded_val)
        self.network.send_packet(C_MOVE_PAWN, body)

if __name__ == '__main__':
    # Application startup
    app = IsolaClientApp()
    app.run()