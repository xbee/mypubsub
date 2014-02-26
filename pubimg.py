#!/usr/bin/python

import argparse
import errno
import os
import sys
import time
import datetime
import mosquitto
from threading import Timer
import Image
from cStringIO import StringIO
from screencap import *

# parser = argparse.ArgumentParser()
# parser.add_argument('-D', '--Directory', action='store', dest='directory', help='Directory/path to save images. Default is ./')
# parser.add_argument('-d', '--Duration', action='store', dest='duration', required=True, type=int, help='Duration of runtime for script in seconds by default.')
# parser.add_argument('-p', '--Period', action='store', dest='period', required=True, type=int, help='Interval between screenshots in seconds by default.')

class RepeatingTimer(object):
	"""
	USAGE:
	from time import sleep
	def myFunction(inputArgument):
		print(inputArgument)
	
	r = RepeatingTimer(0.5, myFunction, "hello")
	r.start(); sleep(2); r.interval = 0.05; sleep(2); r.stop()
	"""
 
	def __init__(self,interval, function, *args, **kwargs):
		super(RepeatingTimer, self).__init__()
		self.args = args
		self.kwargs = kwargs
		self.function = function
		self.interval = interval
 
	def start(self):
		self.callback()
		
	def stop(self):
		self.interval = False
		
	def callback(self):
		if self.interval:
			self.function(*self.args, **self.kwargs)
			Timer(self.interval, self.callback, ).start()


def snapShot2(targetDir):
    """
    Takes a screenshot of current desktop. Saves image with timestap as filename.
    """

    timeStamp = targetDir + datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S")

    # import - saves  any visible window on an X server and outputs it as an 
    #      image file. You can capture a single window, the entire screen, 
    #      or any rectangular portion of the screen.
    command = "import -window root " + timeStamp + ".png"
    if os.system(command) != 0:
        # command failed
        sys.stderr.write(datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S") + '\timport command failed.\n')
        sys.exit()

    return timeStamp

def snapShot():
  s = screengrab()
  screen = s.screen()
  out = StringIO()
  screen.save(out, format="JPEG", quality=50)
  ct = out.getvalue()
  out.close()
  return ct

def fn2topic(fn):
  b = fn.split('.')
  b.reverse()
  topic = b[0]+'/'+b[1]
  return topic  

client = mosquitto.Mosquitto("image-send")
client.connect("127.0.0.1", 1883)

def pubimg():
  imagestring = snapShot()
  byteArray = bytes(imagestring)
  t = datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S")
  client.publish("photo/jpg/%s" % t, byteArray ,1)


if __name__ == '__main__':
  r = RepeatingTimer(5.0, pubimg)
  r.start()


