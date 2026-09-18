# Server-to-Server Connectivity

The Connect 4 program now supports an optional TCP connection between two running game-server instances.

## Run locally

Start the first copy of the program:

1. Enter the two local player names.
2. Choose `1 - Listen for another Connect 4 server`.
3. Enter a TCP port such as `5555`.

Start a second copy of the program:

1. Enter the player names.
2. Choose `2 - Connect to an existing Connect 4 server`.
3. Enter `127.0.0.1` as the host when both programs are on the same computer.
4. Enter the same port used by the listening server, such as `5555`.

For two different computers, use the listening computer's reachable IP address instead of `127.0.0.1`. The selected TCP port must also be allowed through any firewall or network rules.

## Messages exchanged

The connection uses newline-delimited TCP messages. The game sends events such as:

- `SERVER_HELLO|Connect4 peer online`
- `MOVE|<player>|X|column=<n>|board=<42-character-board>`
- `WIN|<player>|X`
- `DRAW|board=<42-character-board>`
- `RESTART`
- `GAME_END`

The receiving game displays received messages with the `[Peer Server]` prefix.

This feature adds peer server communication without changing the existing two-player local gameplay.
