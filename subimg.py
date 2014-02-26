#!/usr/bin/python

import argparse
import errno
import os
import sys
import time
import datetime
import mosquitto
from threading import Timer
from cStringIO import StringIO
from screencap import *
import mosquitto

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

def snapShot():
  s = screengrab()
  screen = s.screen()
  out = StringIO()
  screen.save(out, format="JPEG")
  ct = out.getvalue()
  out.close()
  return ct

def pubimg():
  imagestring = snapShot()
  byteArray = bytes(imagestring)
  t = datetime.datetime.now().strftime("%Y-%m-%d_%H%M%S")
  client.publish("photo/jpg/%s" % t, byteArray ,1)

def parse_cmd(topic, content):
  if topic.startswith('rtmap/cmd'):
    if content.startswith('screen'):
      r = RepeatingTimer(5.0, pubimg)
      r.start()

def on_message(mosq, obj, msg):
  print "Received an image %s, len: %d" % (msg.topic, len(msg.payload))
  n = msg.topic.split('/')
  fn = "%s.%s" % (n[-1], n[-2])
  with open('received/%s' % fn, 'wb') as fd:
    fd.write(msg.payload)

client = mosquitto.Mosquitto("image-rec")
client.connect("127.0.0.1", 1883)
client.subscribe("photo/#",1)
client.on_message = on_message

while True:
   client.loop(2)
   time.sleep(1)
