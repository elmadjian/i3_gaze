import sys
import os
import subprocess
import socket
import numpy as np
from pynput.keyboard import Key, Controller, KeyCode
from threading import Thread


class Tracker():

    def __init__(self):
        self.stop = False
        subprocess.Popen(['./stream/streamer'])
        self.sock = self.create_connection('127.0.0.1', 9998)
        self.stream_thread = None
        self.keyboard = Controller()
        self.w, self.h = 1920, 1080
        self.gx, self.gy = 0,0
        self.hx, self.hy, self.hz = 0,0,0
        self.rx, self.ry, self.rz = 0,0,0


    def create_connection(self, ip, port):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.bind((ip, port))
        return sock

    def stream_data(self):
        self.stream_thread = Thread(target=self._stream_loop, args=())
        self.stream_thread.start()

    def _stream_loop(self):
        while not self.stop:
            data,_ = self.sock.recvfrom(64)
            data = data.decode('ascii', 'replace').split('\n')
            coord = data[0].split('~')
            if coord[0] == 'g': #gaze data
                self.gx = float(coord[1])
                self.gy = float(coord[2])
            if coord[0] == 'h': #head data
                self.hx = float(coord[1])
                self.hy = float(coord[2])
                self.hz = float(coord[3])
                self.rx = float(coord[4])
                self.ry = float(coord[5])
                self.rz = float(coord[6])
    
    def calculate_head_vector(self):
        hx,hy,hz = self.hx, self.hy, self.hz
        rx,ry,rz = self.rx, self.ry, self.rz
        R = self._get_rotation_matrix(rx, ry, rz)
        T = self._get_transform_matrix(hx, hy, hz, R)
        vec = np.array([0.5,1,1])
        print(np.dot(R, vec))


    def _get_rotation_matrix(self, rx, ry, rz):
        R_x = np.array([[1, 0,  0],
                        [0, np.cos(rx), -np.sin(rx)],
                        [0, np.sin(rx),  np.cos(rx)]])
        R_y = np.array([[np.cos(ry), 0, np.sin(ry)],
                        [0, 1, 0],
                        [-np.sin(ry), 0, np.cos(ry)]])
        R_z = np.array([[np.cos(rz), -np.sin(rz), 0],
                        [np.sin(rz), np.cos(rz), 0],
                        [0, 0, 1]])
        return np.dot(R_z, np.dot(R_y, R_x))


    def _get_transform_matrix(self, x, y, z, R):
        R = np.hstack((R, np.array([[x],[y],[z]])))
        T = np.vstack((R, np.array([0,0,0,1])))
        return T


#==========================================
if __name__=='__main__':
    tracker = Tracker()
    tracker.stream_data()
    while True:
        tracker.calculate_head_vector()

