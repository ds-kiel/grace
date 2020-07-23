import os,sys
import threading
import time
import pathlib,posixpath

from flask import Flask
from flask_restful import Resource, Api
import requests

from ctypes import *
from enum import Enum

# Use FFI because we don't want to deal with pythons garbage collector
transmitter_so_file = pathlib.PurePath.joinpath( pathlib.Path(__file__).parent.absolute(), "../libtransmitter.so")
transmitter_functions = CDLL(str(transmitter_so_file))
transmitter_functions.transmitter_send_pulse.restype = c_ulonglong

app = Flask(__name__)
api = Api(app)


class NodeState(Enum):
    UNKNOWN = 0
    IDLE = 1
    COLLECTING = 2
    PENDING_SYNC = 3

class Action(Enum):
    START = 1
    STOP = 2
    TIMESTAMP = 3


# TODO read from file
# TODO handle 404, no route to host etc.
nodes = ["raspi20", "raspi05", "raspi07"]
collector_node_states = {}
collector_node_start_timestamp = {}
collector_node_stop_timestamp = {}

for node in nodes:
    collector_node_states[node] = NodeState.UNKNOWN

lock = threading.Lock()

# TODO handle nodes that are already collecting -> e.g. shutdown?
# Define a function for the thread
def start_nodes():
    global collector_node_states
    global collector_node_start_timestamp

    lock.acquire()
    for node in collector_node_states.keys():
        success = requests.get("http://{}:5000/start".format(node)).content
        print("success: {}".format(success))
        if success:
            collector_node_states[node] = NodeState.PENDING_SYNC

    while NodeState.PENDING_SYNC in list(collector_node_states.values()):
        # After every body is informed send out pulse
        timestamp = transmitter_functions.transmitter_send_pulse()
        time.sleep(0.5) # probably not needed just for safety for now

        # check all pending nodes whether they received the pulse
        for node,old_state in collector_node_states.items():
            if old_state == NodeState.PENDING_SYNC:
                new_state = NodeState(int(requests.get("http://{}:5000/state".format(node)).content))
                collector_node_states[node] = new_state
                if new_state == NodeState.COLLECTING:
                    collector_node_start_timestamp[node] = timestamp;

    lock.release()

# TODO handle the case that the node is IDLE
def stop_nodes():
    global collector_node_states
    global collector_node_stop_timestamp
    lock.acquire()

    for node in collector_node_states.keys():
        success = requests.get("http://{}:5000/stop".format(node)).content
        print("success: {}".format(success))
        if success:
            collector_node_states[node] = NodeState.PENDING_SYNC

    while NodeState.PENDING_SYNC in list(collector_node_states.values()):
        # After every body is informed send out pulse
        timestamp = transmitter_functions.transmitter_send_pulse()
        time.sleep(0.5) # probably not needed just for safety for now

        # check all pending nodes whether they received the pulse
        for node,old_state in collector_node_states.items():
            if old_state == NodeState.PENDING_SYNC:
                new_state = NodeState(int(requests.get("http://{}:5000/state".format(node)).content))
                collector_node_states[node] = new_state
                if new_state == NodeState.IDLE:
                    collector_node_stop_timestamp[node] = timestamp;
    lock.release()



# timestamp_action_thread = threading.Thread(target=start_nodes)

class State(Resource):
    def get(self):
        states = {}
        for node in collector_node_states.keys():
            res = requests.get("http://{}:5000/state".format(node)).content
            states[node] = NodeState(int(res))
        return str(states)


class Factors(Resource):
    global collector_node_start_timestamp
    global collector_node_stop_timestamp

    def get(self):
        factors = {}
        for node in nodes:
            factors[node] = (collector_node_start_timestamp[node], collector_node_stop_timestamp[node])
        return str(factors)

class Start(Resource):
    start_action_thread = None

    def get(self):
        # add timed event source to check acknowledgments
        try:
            if Start.start_action_thread == None or not Start.start_action_thread.is_alive():
                Start.start_action_thread = threading.Thread(target=start_nodes)
                Start.start_action_thread.start()
            else:
                return('start job already running')
        except:
            return "Could not start thread"
        return "Starting collectors. Check status at /state"

class Stop(Resource):
    stop_action_thread = None

    def get(self):
        # add timed event source to check acknowledgments
        try:
            if Stop.stop_action_thread == None or not Stop.stop_action_thread.is_alive():
                Stop.stop_action_thread = threading.Thread(target=stop_nodes)
                Stop.stop_action_thread.start()
            else:
                return('stop job already running')

        except:
            return "Could not start thread"
        return "Stopping collectors. Check status at /state"

api.add_resource(Start, '/start')
api.add_resource(Stop, '/stop')
api.add_resource(State, '/state')
api.add_resource(Factors, '/factors')


if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True)
