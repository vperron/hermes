#!/usr/bin/env python

import json
import zlib
from flask import Flask, request

app = Flask(__name__)

import logging
log = logging.getLogger('werkzeug')
# log.setLevel(logging.ERROR)

output_file = open('output.log', 'a')


@app.route('/datastreams/<dsuid>/measurements', methods=['POST'])
def postdatastream(dsuid):
    d = 0
    for k, v in request.headers:
        d += len(k) + len(v)
    print "------------- new data, request size : %d" % (d + len(request.data))
    try:
        if request.headers['Content-Encoding'] == 'gzip':
            bufsize = request.headers['X-Decoded-Length']
            # No handling of max size yet, should use Decompress object
            uncompressed = zlib.decompress(request.data)
            # print "Uncompressed size: %d" % len(uncompressed)
            data = json.loads(uncompressed)
            lines = [",".join((
                point['ts'], point['mac'], point['manuf'], str(point['rssi']), point['bssid'], point['ssid'])
            ) for point in data]
            output_file.writelines(lines)
            for line in lines:
                print line
            print "Number of records: ", len(lines)
        else:
            print request.data

    except zlib.error:
        print "Could not decompress request data."
    except KeyError:
        print request.data

    return "OK"


if __name__ == '__main__':
    app.debug = True
    app.run(host='0.0.0.0', port=8008)
