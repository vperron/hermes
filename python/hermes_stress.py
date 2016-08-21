#!/usr/bin/env python

"""Hermes Stresstest.

Usage:

    hermes_stress.py

Options:
    -h, --help                Show this screen.
    -n N, --num=N             Sends N messages (default 1000)
    -e ENDP, --endpoint=ENDP  Set zmq endpoint (default tcp://localhost:10070)
"""

import sys
import getopt
import zmq
import datetime
import random
import string
from time import sleep

DEFAULT_NUM = 1000
DEFAULT_ENDPOINT = 'tcp://localhost:10070'

def generate_random_msg():
    """  
    Generates random SNOW-like multipart messages in the form :
    timestamp mac_addr_hash mac_addr_prefix dBm [ssid]
    """
    mac_addr_hash = str(hex(random.getrandbits(32)))[2:-1]
    mac_addr_prefix = str(hex(random.getrandbits(24)))[2:-1]
    dBm = str(random.randint(-100,0))
    choice = random.randint(0,1)
    if choice:
        maxchars = random.randint(0,32)
        ssid = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in xrange(maxchars))
        return [mac_addr_hash, mac_addr_prefix, dBm, ssid]
    else:
        return [mac_addr_hash, mac_addr_prefix, dBm]

def generate_timestamp():
    time = datetime.datetime.utcnow()
    time = time + datetime.timedelta(seconds=random.randint(-30,30))
    time = time + datetime.timedelta(minutes=random.randint(-30,30))
    time = time + datetime.timedelta(hours=random.randint(-12,12))
    return time.strftime("%Y-%m-%dT%H:%M:%S+00:00")

def run(num, endpoint):
    context = zmq.Context()
    socket = context.socket(zmq.PUSH)
    socket.connect (endpoint)
    count = 0

    for i in xrange(num):
        resend = random.randint(1,5)
        timestamps = [generate_timestamp() for i in xrange(random.randint(1,5))]
        base_msg = generate_random_msg()
        msgs = [ [ts]+base_msg for ts in timestamps]
        for msg in msgs:
            print msg
            count += 1
            socket.send_multipart(msg)
        #sleep(0.1)
    
    print "Send %d messages, of %d unique" % (count, num)
    sleep(10)

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hn:e:', ['help', 'num=', 'endpoint='])
    except getopt.error, msg:
        print msg
        print "for help use --help"
        sys.exit(2)

    num = DEFAULT_NUM
    endpoint = DEFAULT_ENDPOINT
    for o, a in opts:
        if o in ("-h", "--help"):
            print __doc__
            sys.exit(0)
        elif o in ("-n", "--num"):
            num = int(a)
        elif o in ("-e", "--endpoint"):
            endpoint = a

    run(num, endpoint)


if __name__ == "__main__":

    main()
