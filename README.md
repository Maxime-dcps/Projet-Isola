# Isola - A Multiplayer Isolation Game

This project is a client-server implementation of the abstract strategy game "Isolation", featuring online multiplayer and a planned AI opponent.

## How to Play Isolation

Isolation is a two-player turn-based strategy game.

-   **Goal:** Be the last player who can make a move.
-   **On Your Turn:** You must perform two actions:
    1.  **Move:** Move your pawn to any empty adjacent square (horizontally, vertically, or diagonally).
    2.  **Remove:** After moving, remove any empty square from the board permanently.
-   **Losing Condition:** You lose if you are trapped and cannot make a legal move.

## Features

-   Online 2-player gameplay over a network.
-   Graphical user interface built with Pygame CE.
-   Server-side validation of game logic.
-   A computer opponent to play against.

## Technology Stack

-   **Backend (Server):**
    -   Language: **C**
    -   OS: Linux
    -   Networking: TCP/IP Sockets

-   **Frontend (Client):**
    -   Language: **Python**
    -   UI: **Pygame Community Edition**

## AI Opponent

A core goal of this project is to create a challenging AI opponent.

-   **Algorithm:** The AI will use the **Minimax algorithm with Alpha-Beta Pruning** to search for the best possible move.
-   **Heuristics:** To evaluate board positions, it will use a **heuristic evaluation function**.
