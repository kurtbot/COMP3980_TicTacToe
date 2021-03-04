from ast import Str
from ctypes import *      
from fsm.fsm import fsm_run
from fsm.fsm import Environment
from fsm.fsm import Transition
from fsm.fsm import DefaultStates
from enum import Enum

import socket       

# Create a socket object  
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) 

debug = True

class States(Enum):
    SETUP = DefaultStates.STATE_START
    READ_SERVER = 3
    READ_INPUT = 4
    SEND = 5
    UPDATE = 6
    ERROR = 7
    END = 8

class Tile:
    def __init__(self, tile_id, tile_owner) -> None:
        self.id = tile_id
        self.owner = tile_owner

class Globals(Environment):
    def __init__(self, f_state, t_state) -> None:
        super().__init__(f_state, t_state)
        self.id = -1
        self.setup = True
        self.move = -1
        self.server_input = -1
        self.target_fd = -1
        self.board = []
        for i in range(10):
            self.board.append(Tile(-4, -4))

# Connecting to the server
def setup(env):
    if(debug):
        print("\n========== setup state ==========")
    e = env        

    # Define the IP address and port on which you want to connect  
    # ip_addr = '108.172.15.124'
    local = '127.0.0.1'
    port = 8080

    # connect to the server on local computer  
    s.connect((local, port))  

    print("loading...")
    print("Connecting to server")

    s.send("b0".encode())

    return States.READ_SERVER

def read_server(env):
    if(debug):
        print("\n========== read server state ==========")
    inp = ""

    # while(not inp.isdigit()):
    #     # inp = input("waiting for server input: ")
    #     print("reading")
    #     inp = s.recv(1024)
    #     print("read ", inp)

    inp = s.recv(1024)
    if(debug):
        print("Client received: ", inp)
    # split input: 10 <client_fd>
    
    if(int(str(inp.decode()).split(" ")[0]) == 10):
        env.target_fd = int(str(inp.decode()).split(" ")[1])
        if (len(str(inp.decode()).split(" ")) == 3 and env.setup):
            server_input = 9
            env.id = 1  # player 1
            env.setup = False
    else:
        server_input = int(str(inp.decode()).split(" ")[0])

    print(env.setup)

    if (server_input < 9 and env.setup):
        env.id = 0  # player 2
        env.setup = False
    
    env.server_input = server_input

    # Verify Server Data
    if(server_input < 9):
        return States.UPDATE
    elif (server_input == 9):
        return States.READ_INPUT
    elif (server_input == 10):
        print("Opponent found")
        return States.READ_SERVER
    elif (server_input == 11):
        print("Please wait for an opponent")
        return States.READ_INPUT
    elif (server_input >= 12):
        return States.END
    else:
        return States.ERROR
    
def read_input(env):
    if(debug):
        print("\n========== read input state ==========")
    inp = ""

    data = ""
    i = 0

    while(not inp.isdigit()):
        inp = input("Input Move [" + ('X','O')[env.id == 1] + "]: ")

    client_input = str(inp)

    env.move = client_input
    return States.SEND

def send(env):
    data = ""
    i = 0
    for i in range(9):
        data += (str(env.board[i].id), '_')[env.board[i].id == -4]

    formattedMove = str(env.target_fd) + " " + data + " " + str(env.move) + " " + str(env.id)
    
    if(debug):
        print("\n========== send state ==========")
        print("sending " + formattedMove + " to server")
    s.send(str(formattedMove).encode())
    return States.READ_SERVER

def update(env):
    if(debug):
        print("\n========== update state ==========")

    env.board[env.server_input].owner = (0, 1) [env.id == 0]

    env.board[env.move].owner = env.id

    if(debug):
        for i in range(9):
            print("", env.board[i].id, env.board[i].owner)

        print("9: ", env.board[9].id, env.board[9].owner)

    for i in range(9):
        c = "?"

        if(env.board[i].owner == 1):
            c = "O"
        elif (env.board[i].owner == 0):
            c = "X"

        print(c, end='')
        if (i == 2 or i == 5 or i == 8):
            print('\n', end='')
    
    return States.READ_INPUT

# Handles game end conditions - Win (12), Lose (13), and Tie (14)
def end(env):
    if(env.id == 1 and env.server_input == 12):
        # PLAYER 1 WIN
        print("You Win")
    elif(env.id == 1 and env.server_input == 13):
        # PLAYER 1 LOSE
        print("You Lose")
    elif(env.id == 0 and env.server_input == 12):
        # PLAYER 2 LOSE
        print("You Lose")
    elif(env.id == 0 and env.server_input == 13):
        # PLAYER 2 WIN
        print("You Win")
    elif(env.server_input == 14):
        # DRAW
        print("Draw")
    else:
        # ERROR
        return States.ERROR

    return DefaultStates.STATE_EXIT
    
    
def common_error(env):
    print("error found")
    return DefaultStates.STATE_NULL

def main():
    print("[TIC TAC TOE] by Brian and Kurt")

    tran_table = [
        Transition(DefaultStates.STATE_INIT, States.SETUP, setup),
        Transition(States.SETUP, States.READ_SERVER, read_server),
        Transition(States.SETUP, States.ERROR, common_error),
        Transition(States.READ_SERVER, States.READ_INPUT, read_input),
        Transition(States.READ_SERVER, States.UPDATE, update),
        Transition(States.READ_SERVER, States.ERROR, common_error),
        Transition(States.READ_INPUT, States.SEND, send),
        Transition(States.READ_INPUT, States.ERROR, common_error),
        Transition(States.UPDATE, States.READ_INPUT, read_input),
        Transition(States.UPDATE, States.ERROR, common_error),
        Transition(States.SEND, States.READ_SERVER, read_server),
        Transition(States.READ_SERVER, States.READ_SERVER, read_server),
        Transition(States.SEND, States.ERROR, common_error),
        Transition(States.READ_SERVER, States.END, end),
        Transition(States.END, DefaultStates.STATE_EXIT, None),
        Transition(States.END, States.ERROR, common_error),
        Transition(DefaultStates.STATE_NULL, DefaultStates.STATE_NULL, None),
    ]

    glbl = Globals(DefaultStates.STATE_INIT, States.SETUP)
    start_state = DefaultStates.STATE_INIT
    next_state = States.SETUP
    fsm_run(glbl, start_state, next_state, tran_table)

main()
s.close()