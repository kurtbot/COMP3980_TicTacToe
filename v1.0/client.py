# Import socket module  
import socket     
from ctypes import *        
  
# Create a socket object  
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)         
  
# Define the port on which you want to connect  
port = 8080

# connect to the server on local computer  
s.connect(('127.0.0.1', port))  
  
# receive data from the server
data = s.recv(1024)  
print (str(data, 'utf-8')) 
# close the connection  
s.close()
