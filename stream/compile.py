import os
import shutil

if os.path.exists("build"):
    shutil.rmtree("build")
path = "mkdir build && cd build && cmake .. && make && mv streamer ../"
out = os.popen(path)
print(out.read())       

