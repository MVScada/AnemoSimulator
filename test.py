#!/usr/bin/env python
# -*- coding: UTF-8 -*-
#
#
import sys
import os
sys.path.insert(0, './')
import time
import datetime
import binascii
import serial
#import threading
import traceback
#import signal
#import glob
#import pprint
#import string
#import commands
import bitstring
import struct
import logging
import atexit
import netifaces

import simplejson as json
import urllib
import urllib2
import base64

LOGGER_FILE = '/tmp/anemosimu.log'
LOCK_FILE = '/tmp/anemosimu.lock'
LOCK_MODEM = '/tmp/modem.lock'
VERBOSE = True
URL = "http://mvscada.com/apianemo/"
HEADERS = {'User-Agent': "MVScada AnemoSimu"}


def lock_file():
    if not os.path.exists(LOCK_FILE):
        lg.debug("Creating LOCK...")
        fd = open(LOCK_FILE, 'w')
        fd.write("\n")
        fd.close()
        return True
    lg.debug("LOCK exists, exit...")
    return False

def lock_modem():
    open(LOCK_MODEM, 'w').close()

def unlock_modem():
    if os.path.exists(LOCK_MODEM):
        os.unlink(LOCK_MODEM)


def clean_lock():
    if os.path.exists(LOCK_FILE):
        lg.debug("Cleaning LOCK...\n\n")
        os.unlink(LOCK_FILE)
    if os.path.exists(LOCK_MODEM):
        os.unlink(LOCK_MODEM)


logging.basicConfig(level=logging.DEBUG,
                    format='%(asctime)s %(message)s',
                    filename=LOGGER_FILE,
                    filemode='a')
lg = logging.getLogger()

if "debug" in sys.argv:
    ch = logging.StreamHandler(sys.stdout)
    ch.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s %(message)s')
    ch.setFormatter(formatter)
    lg.addHandler(ch)

def sendRequest(data={}, method=None):
    local_indent = ''
    if VERBOSE:
        local_indent = 4 * ' '
    localURL = URL
    if method:
        localURL = URL + method + "/"
    #
    data = urllib.urlencode(data)
    
    if 'tun0' in netifaces.interfaces():
        HEADERS['VPN'] = netifaces.ifaddresses('tun0')[2][0]['addr']
    else:
        HEADERS['VPN'] = 'n/a'
    
    req = urllib2.Request(localURL, data, HEADERS)
    response = None
    raw = ""
    #lg.debug("sendRequest() url=%s" % localURL)
    try:
        raw = urllib2.urlopen(req).read()
    except Exception, err:
        lg.error("Exception loading API, err=%s" % err)
        return response

    try:
        response = json.loads(raw)
        if "debug" in response:
            txt = json.dumps(response, indent=local_indent)
            lg.debug(txt)
    except Exception, err:
        #lg.error("Exception, err: %s json:\n%s" % (err, response))
        lg.debug("sendRequest() received raw\n")
        lg.debug("="*80)
        lg.debug(raw)
        lg.debug("="*80)
        #traceback.print_exc(file=sys.stdout)
    #
    return response


def upload_status(m, status):
    res = sendRequest({'seguidor': base64.encodestring(json.dumps(m)),
                       'status': status},
                      'upload-status')
    lg.debug(res)


def printHex(data):
    tmp = []
    info = binascii.hexlify(data)
    for i in range(0, len(info), 2):
        tmp.append(info[i:i + 2])
    return " ".join(tmp)

def txrx(*msg):
    tx = struct.pack('BB', 0x40, len(msg))
    #print "%s (%s)" %(binascii.hexlify(tx), tx)
    for i in msg:
        #print i
        if isinstance(i, int):
            tx += struct.pack('B', i)
        else:
            for k in i:
                tx += struct.pack('B', k)
    crc = 0
    #print msg, len(msg)
    for i in range(0, len(tx)):
        crc += ord(tx[i])
    if crc > 255:
        crc = crc % 256
    #print "CRC", crc
    tx += struct.pack('B', crc)
    #
    #
    data = serial.read(20)
    if data:
        lg.debug("  RX <= %s (%s)" %(printHex(data), data))
    #
    #data = binascii.hexlify(tx)
    #print "TX => ", data
    lg.debug("TX => %s" %(printHex(tx)))
    serial.write(tx)
    #
    #
    data = serial.read(20)
    tmp={}
    if len(data)> 0 and ord(data[0]) == 0x23:
        tmp['result'] = ord(data[2])
        tmp['status'] = ord(data[3])
        tmp['freq'] = ord(data[4]) * 256 + ord(data[5])
        tmp['temp'] = (ord(data[6]) * 256 + ord(data[7])) / 10.0

    tmp['raw']=data
    
    lg.debug("  RX <= %s %s" %(printHex(data), tmp))
    return tmp


def freq2hex(freq):
    ret = []
    #ret.append( int("%02x" %(freq >>8 & 0xFF), 16) )
    #ret.append( int("%02x" %(freq     & 0xFF), 16) )
    ret.append( (freq >>8 & 0xFF) )
    ret.append( (freq     & 0xFF) )
    return ret
    #"%02x %02x" %(0x31 >>8 & 0xFF, 0x31 & 0xFF)




serial = serial.Serial("/dev/ttyUSB0", 9600, timeout=0.3)
#serial = serial.Serial("/dev/ttyUSB0", 9600)
print serial


def processActions(txt):
    if "|" in txt:
        tmp=txt.split("|")
        for a in tmp:
            result=processActions(a)
            print result
        return result
    if " " in txt:
        tmp=txt.split(" ")
        print tmp
        freq=freq2hex( int(tmp[1]) )
        print freq
        result = txrx( int('0x' + tmp[0], 16), freq )
    else:
        result = txrx( int('0x' + txt, 16) )
    return result

sys.exit(0)

modems  = sendRequest()
print modems
for m in modems:
    if m['end'] is not None:
        continue
    #lg.debug(m)
    lg.info("%s (%s) => %s" % (m['inst_id'], m['phone'], m['action']) )
    #
    #
    #FIXME llamada de telefono
    #upload_status(m, 'Realizando llamada...')
    #
    #
    # action is hex value
    #action = bitstring.BitArray('0x' + m['action']).uint
    #action = int('0x' + m['action'], 16)
    action = int('0x' + "31", 16)
    result = txrx( action )
    print result

    time.sleep(1)

    action = int('0x' + "32", 16)
    result = txrx( action )

    print result

    del(result['raw'])

    # m['status']=""
    # d = datetime.datetime.now()
    # m['end'] = txt = "%04d-%02d-%02d %02d:%02d:%02d" % (d.year, d.month, d.day, d.hour, d.minute, d.second)

    # res = sendRequest({'data':     base64.encodestring(json.dumps(result)),
    #                    'seguidor': base64.encodestring(json.dumps(m))},
    #              'upload-result')
    # #lg.debug(res)




sys.exit(0)


#send( 0x31 ); time.sleep(1); send( 0x32 ); time.sleep(1)

# send( 0x33 )
# time.sleep(1)

# send( 0x32 )
# time.sleep(1)

# "%02x %02x" %(0x31 >>8 & 0xFF, 0x31 & 0xFF)




send( 0x35, freq2hex(13) )
#send( 0x36 )

#send( 0x37 )

# mode = 0x31
# msg = struct.pack('B', mode)
# txrx(msg)

# time.sleep(1)

# mode = 0x32
# msg = struct.pack('B', mode)
# txrx(msg)

# time.sleep(1)
# # arrancar con frecuencia 255 Hz
# mode = 0x35
# msg = struct.pack('>BH', mode, 69)
# txrx(msg)

# mode = 0x32
# msg = struct.pack('B', mode)
# txrx(msg)

#serial.close()
