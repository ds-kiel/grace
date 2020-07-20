from flask import Flask
from flask_restful import Resource, Api

from ctypes import *

# Use FFI because we don't want to deal with pythons garbage collector
transmitter_so_file = "../build/lib/transmitter.so"
transmitter_functions = CDLL(transmitter_so_file)

app = Flask(__name__)
api = Api(app)

collector_node_states = {}

class HelloWorld(Resource):
    def get(self):
        return {'hello': 'world'}

class Start(Resource):
    def get(self):
        # add timed event source to check acknowledgments
        timestamp = transmitter_functions.transmitter_send_pulse();
        return {'Ok!'}

api.add_resource(HelloWorld, '/')

if __name__ == '__main__':
    app.run(debug=True)
