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

debug = False

class States(Enum):
    SETUP = DefaultStates.STATE_START
    WAIT_SERVER = 2
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
        self.board = []
        self.uid = 0
        for i in range(10):
            self.board.append(Tile(-4, -4))


def setup(env):
    if(debug):
        print("\n========== setup state ==========")
    e = env        
  
    # Define the IP address and port on which you want to connect  
    local = '127.0.0.1'
    port = 2034

    # connect to the server on local computer  
    s.connect((local, port))  

    print("loading...")
    print("Connecting to server")

    confirmation = 1
    ruleset = 1
    payLen = 2
    protocolVer = 1
    gId = 1

    packet = [e.uid, e.uid, e.uid, e.uid, confirmation, ruleset, payLen, protocolVer, gId]
    # print(packet)
    s.sendall(bytes(packet))

    # s.send("ping".encode())
    return States.WAIT_SERVER

def wait_server(env):
    if(debug):
        print("\n========== wait server state ==========")
    msg_type = int.from_bytes(s.recv(1), 'big')
    context = int.from_bytes(s.recv(1), 'big')
    payload_len = int.from_bytes(s.recv(1), 'big')
    env.uid = int.from_bytes(s.recv(payload_len), 'little')

    print(env.uid)

    # TODO: error handling

    return States.READ_SERVER


def read_server(env):
    if(debug):
        print("\n========== read server state ==========")
    inp = ""

    inp = s.recv(1024)
    
    message = get_message(s)

    # if (message["header"]["msg_type"] == 20):
    #     env.id = (message["payload"][0])    # 1 = X, 2 = O

    if(debug):
        print("Client received: ", inp)
    server_input = int(inp)

    # if (server_input < 9 and env.setup):
    #     env.id = 0  # player 2
    # elif (server_input == 9 and env.setup):
    #     env.id = 1  # player 1
    env.setup = False

    env.server_input = server_input

    # Verify Server Data
    if(server_input < 9):
        return States.UPDATE
    elif (server_input == 9):
        return States.READ_INPUT
    elif (server_input >= 10):
        return States.END
    else:
        return States.ERROR
    
def read_input(env):
    if(debug):
        print("\n========== read input state ==========")
    inp = ""

    while(not inp.isdigit()):
        inp = input("Input Move [" + ('X','O')[env.id == 1] + "]: ")

    client_input = int(inp)

    env.move = client_input
    return States.SEND

def send(env):
    if(debug):
        print("\n========== send state ==========")
        print("sending " + str(env.move) + " to server")
    s.send(str(env.move).encode())
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


def end(env):
    if(env.id == 1 and env.server_input == 10):
        # PLAYER 1 WIN
        print("You Win")
    elif(env.id == 1 and env.server_input == 11):
        # PLAYER 1 LOSE
        print("You Lose")
    elif(env.id == 0 and env.server_input == 10):
        # PLAYER 2 LOSE
        print("You Lose")
    elif(env.id == 0 and env.server_input == 11):
        # PLAYER 2 WIN
        print("You Win")
    elif(env.server_input == 12):
        # DRAW
        print("Draw")
    elif(env.server_input == 13):
        # OPPONENT LEFT
        print("Opponent Left")
    else:
        # ERROR
        return States.ERROR

    return DefaultStates.STATE_EXIT
    
def get_message(s):
    header = get_header(s)

    payload_len = header["payload_length"]
    if (payload_len != 0):
        payload = get_payload(s, header)
    else:
        payload = {"payload": None}
    
    message = {'header': header, 'payload': payload}

    return message

def get_header(s):
    msg_type = int.from_bytes(s.recv(1), 'big')
    context = int.from_bytes(s.recv(1), 'big')
    payload_len = int.from_bytes(s.recv(1), 'big')

    header = {"msg_type": msg_type, "context": context, "payload_length": payload_len}

    return header

def get_payload(s, header):
    payload = []
    for i in range(0, header['payload_length']):
        payload.append(int.from_bytes(s.recv(1), 'big'))

    return payload

def common_error(env):
    print("error found")
    return DefaultStates.STATE_NULL

def main():
    print("[TIC TAC TOE] by Brian and Kurt")

    tran_table = [
        Transition(DefaultStates.STATE_INIT, States.SETUP, setup),
        Transition(States.SETUP, States.WAIT_SERVER, wait_server),
        Transition(States.SETUP, States.ERROR, common_error),
        Transition(States.WAIT_SERVER, States.READ_SERVER, read_server),
        Transition(States.READ_SERVER, States.READ_INPUT, read_input),
        Transition(States.READ_SERVER, States.UPDATE, update),
        Transition(States.READ_SERVER, States.ERROR, common_error),
        Transition(States.READ_INPUT, States.SEND, send),
        Transition(States.READ_INPUT, States.ERROR, common_error),
        Transition(States.UPDATE, States.READ_INPUT, read_input),
        Transition(States.UPDATE, States.ERROR, common_error),
        Transition(States.SEND, States.READ_SERVER, read_server),
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