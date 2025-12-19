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

screen = pygame.display.set_mode(SCREEN_SIZE)
pygame.display.set_caption(CAPTION)

MANAGER = pygame_gui.UIManager(SCREEN_SIZE)
CLOCK = pygame.time.Clock()

COLOR_EMPTY = (200, 200, 200)
COLOR_X = (50, 50, 50)
COLOR_P1 = (255, 50, 50)   # Red
COLOR_P2 = (50, 50, 255)   # Blue
COLOR_BLOCKED = (80, 80, 80)  # Dark gray for blocked tiles

# Game board constants
TILE_SIZE = 60
BOARD_COLS = 8
BOARD_ROWS = 6
BOARD_WIDTH = BOARD_COLS * TILE_SIZE   # 480
BOARD_HEIGHT = BOARD_ROWS * TILE_SIZE  # 360

# Center the board in the window (with space for header and indicator)
BOARD_OFFSET_X = (WIDTH - BOARD_WIDTH) // 2   # 160
BOARD_OFFSET_Y = 80  # Space for header

class IsolaClientApp:
    def __init__(self):
        self.running = True
        self.network = NetworkClient()
        self.state = 'CONNECTING'
        self.game_phase = 'MOVE' # MOVE or BLOCK
        self.is_my_turn = False
        self.username = ""
        self.board_data = None  # Store board state for drawing
        self.tiles = [] # Store buttons for the grid
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

    def init_lobby_ui(self, wins=0, losses=0, forfeits=0):
        # On cache le message d'erreur du login s'il existait
        self.status_message.hide()

        # On agrandit le panneau (Passage de 300 à 450 de hauteur)
        self.lobby_panel = pygame_gui.elements.UIPanel(
            relative_rect=pygame.Rect((WIDTH // 4, HEIGHT // 8), (400, 450)),
            manager=MANAGER
        )

        # Welcome message
        self.welcome_label = pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 10), (350, 40)),
            text=f"Welcome, {self.username} !",
            manager=MANAGER, container=self.lobby_panel
        )

        # Simple data label
        self.stats_label = pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 50), (400, 40)),
            text=f"W: {wins} | L: {losses} | F: {forfeits}", manager=MANAGER, container=self.lobby_panel
        )

        # On repositionne les boutons pour qu'ils soient centrés dans le nouveau panneau
        button_width = 200
        start_x = (400 - button_width) // 2  # Centre horizontalement dans le panel de 400px

        self.play_button = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect((start_x, 100), (button_width, 50)),
            text="FIND MATCH", manager=MANAGER, container=self.lobby_panel
        )
        self.players_list_btn = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect((start_x, 170), (button_width, 40)),
            text="Player List", manager=MANAGER, container=self.lobby_panel
        )
        self.change_pw_btn = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect((start_x, 220), (button_width, 40)),
            text="Change Password", manager=MANAGER, container=self.lobby_panel
        )
        self.logout_btn = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect((start_x, 270), (button_width, 40)),
            text="Logout", manager=MANAGER, container=self.lobby_panel
        )

        # Position du statut en bas du panel
        self.lobby_status = pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 380), (360, 40)),
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

    def init_game_ui(self, board_data):
        self.lobby_panel.hide()
        
        # Panel centered, sized to fit board + header + indicator
        panel_width = BOARD_WIDTH + 40  # 20px padding each side
        panel_height = BOARD_HEIGHT + 100  # header + indicator space
        panel_x = (WIDTH - panel_width) // 2
        panel_y = 30
        
        self.game_panel = pygame_gui.elements.UIPanel(
            relative_rect=pygame.Rect((panel_x, panel_y), (panel_width, panel_height)), manager=MANAGER
        )
        self.game_info = pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((0, 5), (panel_width, 35)), text="Game Starting...", manager=MANAGER, container=self.game_panel
        )

        self.tiles = []
        start_x = 20  # Padding inside panel
        start_y = 45  # Below header
        for r in range(BOARD_ROWS):
            row_btns = []
            for c in range(BOARD_COLS):
                btn = pygame_gui.elements.UIButton(
                    relative_rect=pygame.Rect((start_x + c * TILE_SIZE, start_y + r * TILE_SIZE), (TILE_SIZE, TILE_SIZE)),
                    text="", manager=MANAGER, container=self.game_panel
                )
                btn.user_data = (r, c) # Save coordinates in the button
                row_btns.append(btn)
            self.tiles.append(row_btns)
        self.update_board_ui(board_data)

    def update_board_ui(self, board_data):
        self.board_data = board_data  # Store for custom drawing
        for r in range(BOARD_ROWS):
            for c in range(BOARD_COLS):
                val = board_data[r * BOARD_COLS + c]
                btn = self.tiles[r][c]
                btn.set_text("")  # Clear text, we draw custom graphics

        # Update info label
        if self.is_my_turn:
            phase_txt = "Move your pawn" if self.game_phase == 'MOVE' else "Block a tile"
            self.game_info.set_text(phase_txt)
        else:
            self.game_info.set_text("Waiting for opponent...")

    def draw_game_board(self):
        """Draw custom graphics over the game board"""
        if self.state != 'IN_GAME' or self.board_data is None:
            return

        for r in range(BOARD_ROWS):
            for c in range(BOARD_COLS):
                val = self.board_data[r * BOARD_COLS + c]
                
                # Get the button's absolute position on screen
                btn = self.tiles[r][c]
                btn_rect = btn.get_abs_rect()
                
                # Calculate exact center of the button
                center_x = btn_rect.centerx
                center_y = btn_rect.centery
                radius = TILE_SIZE // 2 - 12

                if val == 1:  # Player 1 - Red circle
                    pygame.draw.circle(screen, COLOR_P1, (center_x, center_y), radius)
                    pygame.draw.circle(screen, (180, 30, 30), (center_x, center_y), radius, 3)
                elif val == 2:  # Player 2 - Blue circle
                    pygame.draw.circle(screen, COLOR_P2, (center_x, center_y), radius)
                    pygame.draw.circle(screen, (30, 30, 180), (center_x, center_y), radius, 3)
                elif val == 3:  # Blocked tile - Red X centered
                    cross_size = 15
                    pygame.draw.line(screen, (220, 60, 60), 
                                   (center_x - cross_size, center_y - cross_size),
                                   (center_x + cross_size, center_y + cross_size), 5)
                    pygame.draw.line(screen, (220, 60, 60),
                                   (center_x + cross_size, center_y - cross_size),
                                   (center_x - cross_size, center_y + cross_size), 5)

        # Draw turn indicator below the board
        last_btn_rect = self.tiles[BOARD_ROWS-1][0].get_abs_rect()
        first_btn_rect = self.tiles[0][0].get_abs_rect()
        self.draw_turn_indicator(first_btn_rect.x, last_btn_rect.bottom)

    def draw_turn_indicator(self, board_x, board_bottom):
        """Draw a simple indicator below the board"""
        if self.board_data is None:
            return

        # Position below the board, centered
        indicator_y = board_bottom + 10
        indicator_width = BOARD_WIDTH
        indicator_height = 10
        
        if self.is_my_turn:
            # Green bar
            pygame.draw.rect(screen, (50, 200, 80), (board_x, indicator_y, indicator_width, indicator_height))
        else:
            # Gray bar
            pygame.draw.rect(screen, (80, 80, 80), (board_x, indicator_y, indicator_width, indicator_height))

    # --- LOGIC ---

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
                    self.init_lobby_ui(wins, losses, forfeits)
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
                turn_code = body[48] # 0: Wait, 1: Move, 2: Block
                self.is_my_turn = turn_code > 0
                self.game_phase = 'MOVE' if turn_code == 1 else 'BLOCK'

                if self.state != 'IN_GAME':
                    self.state = 'IN_GAME'
                    self.init_game_ui(board_data)
                else:
                    self.update_board_ui(board_data)

            elif command_id == S_GAME_OVER:
                # 1B result + 1B padding + 2B wins + 2B losses + 2B forfeits = 8 bytes
                result = body[0]
                # body[1] is padding
                wins, losses, forfeits = struct.unpack(">HHH", body[2:8])
                
                self.handle_game_over(result, wins, losses, forfeits)

            elif command_id == S_PLAYER_LIST:
                self.handle_player_list(body)

            elif command_id == S_CHANGE_PASSWORD_RESPONSE:
                status = body[0]
                self.handle_change_password_response(status)

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
                    elif hasattr(self, 'players_list_btn') and event.ui_element == self.players_list_btn:
                        self.network.send_packet(C_GET_PLAYER_LIST)
                    elif hasattr(self, 'close_list_btn') and event.ui_element == self.close_list_btn:
                        self.close_player_list()
                    elif hasattr(self, 'change_pw_btn') and event.ui_element == self.change_pw_btn:
                        self.show_change_password_dialog()
                    elif hasattr(self, 'pw_submit_btn') and event.ui_element == self.pw_submit_btn:
                        self.submit_password_change()
                    elif hasattr(self, 'pw_cancel_btn') and event.ui_element == self.pw_cancel_btn:
                        self.close_change_password_dialog()
                    elif hasattr(self, 'logout_btn') and event.ui_element == self.logout_btn:
                        self.handle_logout()
                    elif self.state == 'IN_GAME' and self.is_my_turn:
                        # Handle grid clicks
                        for r in range(BOARD_ROWS):
                            if event.ui_element in self.tiles[r]:
                                c = self.tiles[r].index(event.ui_element)
                                self.handle_grid_click(r, c)

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
            screen.fill((30, 30, 30))

            MANAGER.update(time_delta)

            MANAGER.draw_ui(screen)

            # Draw custom game graphics on top of UI
            self.draw_game_board()

            pygame.display.update()

        # Cleanup
        self.network.sock.close()
        pygame.quit()
        sys.exit()

    def handle_grid_click(self, r, c):
        encoded = (r << 5) | (c << 2)
        if self.game_phase == 'MOVE':
            self.network.send_packet(C_MOVE_PAWN, struct.pack("B", encoded))
        else:
            self.network.send_packet(C_BLOCK_TILE, struct.pack("B", encoded))

    def handle_game_over(self, result, wins, losses, forfeits):
        # Hide game UI
        if hasattr(self, 'game_panel'):
            self.game_panel.hide()
        
        # Determine result message
        if result == GAME_RESULT_WIN:
            result_text = "YOU WIN!"
        elif result == GAME_RESULT_LOSS:
            result_text = "YOU LOSE!"
        elif result == GAME_RESULT_FORFEIT_WIN:
            result_text = "YOU WIN! (Opponent disconnected)"
        elif result == GAME_RESULT_FORFEIT_LOSS:
            result_text = "YOU LOSE! (Forfeit)"
        else:
            result_text = "Game Over"

        print(f"GAME OVER: {result_text}")

        # Update state and return to lobby
        self.state = 'AUTHENTICATED'
        self.tiles = []

        # Update lobby with new stats
        self.stats_label.set_text(f"W: {wins} | L: {losses} | F: {forfeits}")
        self.lobby_status.set_text(f"Status: {result_text}")
        self.play_button.enable()
        self.lobby_panel.show()

    def handle_player_list(self, body):
        """Handle S_PLAYER_LIST packet and display player list UI"""
        # Each PlayerEntry: 16B username + 2B wins + 2B losses + 2B forfeits + 1B is_online + 1B padding = 24 bytes
        ENTRY_SIZE = 24
        player_count = len(body) // ENTRY_SIZE

        players = []
        for i in range(player_count):
            offset = i * ENTRY_SIZE
            username = body[offset:offset+16].decode('utf-8').rstrip('\x00')
            wins, losses, forfeits = struct.unpack(">HHH", body[offset+16:offset+22])
            is_online = body[offset+22]
            players.append({
                'username': username,
                'wins': wins,
                'losses': losses,
                'forfeits': forfeits,
                'is_online': is_online
            })

        self.show_player_list(players)

    def show_player_list(self, players):
        """Display the player list panel"""
        # Hide lobby panel
        self.lobby_panel.hide()

        # Create player list panel
        panel_width = 500
        panel_height = 450
        self.player_list_panel = pygame_gui.elements.UIPanel(
            relative_rect=pygame.Rect((WIDTH - panel_width) // 2, (HEIGHT - panel_height) // 2, panel_width, panel_height),
            manager=MANAGER
        )

        # Title
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((0, 10), (panel_width, 30)),
            text="Player Rankings",
            manager=MANAGER, container=self.player_list_panel
        )

        # Header row
        header_y = 50
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, header_y), (150, 25)),
            text="Username",
            manager=MANAGER, container=self.player_list_panel
        )
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((180, header_y), (80, 25)),
            text="Wins",
            manager=MANAGER, container=self.player_list_panel
        )
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((260, header_y), (80, 25)),
            text="Losses",
            manager=MANAGER, container=self.player_list_panel
        )
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((340, header_y), (80, 25)),
            text="Forfeits",
            manager=MANAGER, container=self.player_list_panel
        )

        # Player rows
        row_height = 28
        start_y = 85
        max_display = 10  # Max players to display

        for i, player in enumerate(players[:max_display]):
            y = start_y + i * row_height
            
            # Username with color based on online status
            if player['is_online']:
                # Green label for online players
                label = pygame_gui.elements.UILabel(
                    relative_rect=pygame.Rect((20, y), (150, 25)),
                    text=player['username'],
                    manager=MANAGER, container=self.player_list_panel
                )
                label.text_colour = pygame.Color(50, 200, 80)
                label.rebuild()
            else:
                pygame_gui.elements.UILabel(
                    relative_rect=pygame.Rect((20, y), (150, 25)),
                    text=player['username'],
                    manager=MANAGER, container=self.player_list_panel
                )

            # Stats
            pygame_gui.elements.UILabel(
                relative_rect=pygame.Rect((180, y), (80, 25)),
                text=str(player['wins']),
                manager=MANAGER, container=self.player_list_panel
            )
            pygame_gui.elements.UILabel(
                relative_rect=pygame.Rect((260, y), (80, 25)),
                text=str(player['losses']),
                manager=MANAGER, container=self.player_list_panel
            )
            pygame_gui.elements.UILabel(
                relative_rect=pygame.Rect((340, y), (80, 25)),
                text=str(player['forfeits']),
                manager=MANAGER, container=self.player_list_panel
            )

        # Close button
        self.close_list_btn = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect((panel_width // 2 - 75, 400), (150, 35)),
            text="Close",
            manager=MANAGER, container=self.player_list_panel
        )

    def close_player_list(self):
        """Close the player list panel and return to lobby"""
        if hasattr(self, 'player_list_panel'):
            self.player_list_panel.kill()
            del self.player_list_panel
        self.lobby_panel.show()

    def handle_logout(self):
        """Handle logout: close connection, reconnect and return to login screen"""
        # Send disconnect packet to server
        self.network.send_packet(C_DISCONNECT)
        
        # Close current socket and reconnect
        try:
            self.network.sock.close()
        except:
            pass
        
        # Clear receive buffer
        self.network.recv_buffer = b''
        
        # Reconnect
        self.network.connect()
        
        # Hide lobby panel
        self.lobby_panel.kill()
        
        # Reset state - start as CONNECTING to wait for new connection
        self.state = 'CONNECTING'
        self.username = ""
        
        # Show login panel
        self.login_panel.show()
        self.status_message.show()
        self.status_message.set_text("Status: Reconnecting...")
        
        # Clear inputs
        self.username_input.set_text("")
        self.password_input.set_text("")

    def show_change_password_dialog(self):
        """Show dialog to change password"""
        self.lobby_panel.hide()
        
        panel_width = 350
        panel_height = 280
        panel_x = (WIDTH - panel_width) // 2
        panel_y = (HEIGHT - panel_height) // 2
        
        self.pw_dialog_panel = pygame_gui.elements.UIPanel(
            relative_rect=pygame.Rect((panel_x, panel_y), (panel_width, panel_height)),
            manager=MANAGER
        )
        
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((0, 10), (panel_width, 30)),
            text="Change Password",
            manager=MANAGER, container=self.pw_dialog_panel
        )
        
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 50), (120, 25)),
            text="Old Password:",
            manager=MANAGER, container=self.pw_dialog_panel
        )
        self.old_pw_input = pygame_gui.elements.UITextEntryLine(
            relative_rect=pygame.Rect((20, 75), (310, 35)),
            manager=MANAGER, container=self.pw_dialog_panel
        )
        self.old_pw_input.set_text_hidden(True)
        
        pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 115), (120, 25)),
            text="New Password:",
            manager=MANAGER, container=self.pw_dialog_panel
        )
        self.new_pw_input = pygame_gui.elements.UITextEntryLine(
            relative_rect=pygame.Rect((20, 140), (310, 35)),
            manager=MANAGER, container=self.pw_dialog_panel
        )
        self.new_pw_input.set_text_hidden(True)
        
        self.pw_status_label = pygame_gui.elements.UILabel(
            relative_rect=pygame.Rect((20, 185), (310, 25)),
            text="",
            manager=MANAGER, container=self.pw_dialog_panel
        )
        
        self.pw_submit_btn = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect((50, 220), (110, 40)),
            text="Submit",
            manager=MANAGER, container=self.pw_dialog_panel
        )
        self.pw_cancel_btn = pygame_gui.elements.UIButton(
            relative_rect=pygame.Rect((190, 220), (110, 40)),
            text="Cancel",
            manager=MANAGER, container=self.pw_dialog_panel
        )
    
    def submit_password_change(self):
        """Send password change request to server"""
        old_pw = self.old_pw_input.get_text()
        new_pw = self.new_pw_input.get_text()
        
        if not old_pw or not new_pw:
            self.pw_status_label.set_text("Please fill in both fields")
            return
        
        if len(new_pw) < 3:
            self.pw_status_label.set_text("New password too short")
            return
        
        self.pw_status_label.set_text("Processing...")
        self.network.send_change_password(old_pw, new_pw)
    
    def close_change_password_dialog(self):
        """Close password change dialog and return to lobby"""
        if hasattr(self, 'pw_dialog_panel'):
            self.pw_dialog_panel.kill()
            del self.pw_dialog_panel
        self.lobby_panel.show()
    
    def handle_change_password_response(self, status):
        """Handle server response to password change"""
        if not hasattr(self, 'pw_dialog_panel'):
            return
        
        if status == PASSWORD_CHANGE_SUCCESS:
            self.pw_status_label.set_text("Password changed!")
            self.pw_status_label.text_colour = pygame.Color(50, 200, 80)
            self.pw_status_label.rebuild()
            # Return to lobby after success
            self.close_change_password_dialog()
            self.lobby_status.set_text("Status: Password changed successfully!")
        elif status == PASSWORD_CHANGE_WRONG_OLD:
            self.pw_status_label.set_text("Wrong old password!")
            self.pw_status_label.text_colour = pygame.Color(255, 100, 100)
            self.pw_status_label.rebuild()
        else:
            self.pw_status_label.set_text("Error, try again")
            self.pw_status_label.text_colour = pygame.Color(255, 100, 100)
            self.pw_status_label.rebuild()

if __name__ == '__main__':
    # Application startup
    app = IsolaClientApp()
    app.run()