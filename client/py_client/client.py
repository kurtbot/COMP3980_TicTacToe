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

class States(Enum):
    SETUP = DefaultStates.STATE_START
    READ_SERVER = 3
    READ_INPUT = 4
    SEND = 5
    UPDATE = 6
    ERROR = 7

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
        self.board = []
        for i in range(10):
            self.board.append(Tile(-4, -4))


def init_to_start(env):
    print("init to start")
    return DefaultStates.STATE_START

def start_to_start(env):
    print("start to start")
    return DefaultStates.STATE_EXIT

def start_to_exit(env):
    print("start to exit")
    return DefaultStates.STATE_START


def setup(env):
    print("\n========== setup state ==========")
    e = env        
  
    # Define the IP address and port on which you want to connect  
    # ip_addr = '108.172.15.124'
    local = '127.0.0.1'
    port = 6969

    # connect to the server on local computer  
    s.connect((local, port))  

    print("loading...")
    print("Connecting to server")

    return States.READ_SERVER

def read_server(env):
    print("\n========== read server state ==========")
    inp = ""

    # while(not inp.isdigit()):
    #     # inp = input("waiting for server input: ")
    #     print("reading")
    #     inp = s.recv(1024)
    #     print("read ", inp)

    inp = s.recv(1024)
    print("Client received: ", inp)
    server_input = int(inp)

    if (server_input < 9 and env.setup):
        env.id = 0  # player 2
    elif (server_input >= 9 and env.setup):
        env.id = 1  # player 1
    env.setup = False

    env.server_input = server_input

    # Verify Server Data
    if(server_input < 9):
        return States.UPDATE
    elif (server_input >= 9):
        return States.READ_INPUT
    else:
        return States.ERROR
    
def read_input(env):
    print("\n========== read input state ==========")
    inp = ""

    while(not inp.isdigit()):
        inp = input("waiting for client input: ")

    client_input = int(inp)

    env.move = client_input
    return States.SEND

def send(env):
    print("\n========== send state ==========")
    print("sending " + str(env.move) + " to server")
    s.send(str(env.move).encode())
    return States.READ_SERVER

def update(env):
    print("\n========== update state ==========")

    # env.board[env.server_input].id = env.server_input
    if(env.id == 0):
        env.board[env.server_input].owner = 0
    else:
        env.board[env.server_input].owner = 1

    # env.board[env.move].id = env.move
    env.board[env.move].owner = env.id

    for i in range(9):
        print("", env.board[i].id, env.board[i].owner)

    print("9: ", env.board[9].id, env.board[9].owner)

    for i in range(9):
        c = "?"

        if(env.board[i].owner == 0):
            c = "O"
        elif (env.board[i].owner == 1):
            c = "X"
        elif (env.board[i].owner == -1):
            c = "?"

        print(c, end='')
        if (i == 2 or i == 5 or i == 8):
            print('\n', end='')
    
    return States.READ_INPUT

def common_error(env):
    print("error found")
    return DefaultStates.STATE_NULL

def main():
    print("starting main")

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
        Transition(States.SEND, States.ERROR, common_error),
        Transition(DefaultStates.STATE_NULL, DefaultStates.STATE_NULL, None),
    ]

    glbl = Globals(DefaultStates.STATE_INIT, States.SETUP)
    start_state = DefaultStates.STATE_INIT
    next_state = States.SETUP
    fsm_run(glbl, start_state, next_state, tran_table)

main()
s.close()