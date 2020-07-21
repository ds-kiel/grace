import os,sys

from flask import Flask
from flask_restful import Resource, Api

from ctypes import *

# Use FFI because we don't want to deal with pythons garbage collector
transmitter_so_file = os.path.abspath(sys.argv[0] + "/../../build/lib/libtransmitter.so")
transmitter_functions = CDLL(transmitter_so_file)
transmitter_functions.transmitter_send_pulse.restype = c_ulonglong

app = Flask(__name__)
api = Api(app)

collector_node_states = {}

class State(Resource):
    def get(self):
        return collector_node_states

class Start(Resource):
    def get(self):
        # add timed event source to check acknowledgments
        timestamp = transmitter_functions.transmitter_send_pulse()
        return {"Ok!" : str(timestamp)}

api.add_resource(Start, '/start')
api.add_resource(State, '/state')


if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True)
