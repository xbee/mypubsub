#!/usr/bin/env python

import mosquitto
import sys
import pynotify
import datetime
import string

def sendmessage(title, message):
    pynotify.init("image")
    notice = pynotify.Notification(title,message,"/usr/share/icons/gnome/48x48/status/important.png").show()
    return notice

def on_message(mosq, obj, msg):
    sendmessage("Received Message: ","%-20s %s" % (msg.topic, msg.payload))
    # print "%-20s %d %s" % (msg.topic, msg.qos, msg.payload)

    mosq.publish('pong', "Thanks", 0)

def on_publish(mosq, obj, mid):
    pass

cli = mosquitto.Mosquitto("hahah")
cli.on_message = on_message
cli.on_publish = on_publish

'''
cli.tls_set('root.ca',
    certfile='c1.crt',
    keyfile='c1.key')
'''

HOST = "115.28.171.71"
cli.connect(HOST, 1883, 60)
cli.subscribe("rtmap/#", 0)
cli.subscribe("dns/all", 0)
cli.subscribe("nagios/#", 0)

while cli.loop() == 0:
    pass


