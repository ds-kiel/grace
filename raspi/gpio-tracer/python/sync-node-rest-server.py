import os,sys,threading,time,pathlib,posixpath,requests

from flask import Flask
from flask_restful import Resource, Api
from ctypes import *
from enum import Enum

# Use FFI because we don't want to deal with pythons garbage overseer
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
# overseer_node_ids = [2, 3]
overseer_node_ids = [2, 3, 20, 5]
overseer_node_states = {}
overseer_node_timestamps = {}
stop_syncing = False

for overseer_id in overseer_node_ids:
    overseer_node_states[overseer_id] = NodeState.UNKNOWN
    overseer_node_timestamps[overseer_id] = []
lock = threading.Lock()

# TODO handle nodes that are already collecting -> e.g. shutdown?
# TODO handle dead nodes e.g. syncs never arrive
# Define a function for the thread
def start_overseers():
    global stop_syncing
    global overseer_node_states
    global overseer_node_timestamps

    lock.acquire()

    for overseer_id in overseer_node_states.keys():
        success = requests.get("http://raspi{:0>2}:5000/start".format(overseer_id)).content
        print("success: {}".format(success))
        if success:
            overseer_node_states[overseer_id] = NodeState.PENDING_SYNC

    while NodeState.PENDING_SYNC in list(overseer_node_states.values()):
        # After every body is informed send out pulse
        timestamp = transmitter_functions.transmitter_send_pulse()
        time.sleep(0.2) # probably not needed just for safety for now
     # check all pending nodes whether they received the pulse
        for overseer_id,old_state in overseer_node_states.items():
            if old_state == NodeState.PENDING_SYNC:
                new_state = NodeState(int(requests.get("http://raspi{:0>2}:5000/state".format(overseer_id)).content))
                overseer_node_states[overseer_id] = new_state
                if new_state == NodeState.COLLECTING:
                    overseer_node_timestamps[overseer_id].append(timestamp);

    # Sync loop
    while not stop_syncing:
        for overseer_id in overseer_node_states.keys():
            print("informing nodes about incoming sync")
            result = requests.get("http://raspi{:0>2}:5000/sync".format(overseer_id)).content
            print("result: {}".format(success))
            if success:
                overseer_node_states[overseer_id] = NodeState.PENDING_SYNC

        while NodeState.PENDING_SYNC in list(overseer_node_states.values()):
            # After every body is informed send out pulse
            print("sending sync pulse")
            timestamp = transmitter_functions.transmitter_send_pulse()
            time.sleep(0.1) # probably not needed just for safety for now
            # check all pending nodes whether they received the pulse
            for overseer_id,old_state in overseer_node_states.items():
                if old_state == NodeState.PENDING_SYNC:
                    new_state = NodeState(int(requests.get("http://raspi{:0>2}:5000/state".format(overseer_id)).content))
                    overseer_node_states[overseer_id] = new_state
                    if new_state == NodeState.COLLECTING:
                        print("node {} successfull ack".format(overseer_id))
                        overseer_node_timestamps[overseer_id].append(timestamp);

    lock.release()
start_action_thread = None

# TODO handle the case that the overseer_id is IDLE
def stop_overseers():
    global overseer_node_states
    global overseer_node_timestamps
    lock.acquire()

    for overseer_id in overseer_node_states.keys():
        success = requests.get("http://raspi{:0>2}:5000/stop".format(overseer_id)).content
        print("success: {}".format(success))
        if success:
            overseer_node_states[overseer_id] = NodeState.PENDING_SYNC

    while NodeState.PENDING_SYNC in list(overseer_node_states.values()):
        # After every body is informed send out pulse
        timestamp = transmitter_functions.transmitter_send_pulse()
        time.sleep(0.5) # probably not needed just for safety for now

        # check all pending nodes whether they received the pulse
        for overseer_id,old_state in overseer_node_states.items():
            if old_state == NodeState.PENDING_SYNC:
                new_state = NodeState(int(requests.get("http://raspi{:0>2}:5000/state".format(overseer_id)).content))
                overseer_node_states[overseer_id] = new_state
                if new_state == NodeState.IDLE:
                    overseer_node_timestamps[overseer_id].append(timestamp);
    lock.release()

# timestamp_action_thread = threading.Thread(target=start_overseers)

class State(Resource):
    def get(self):
        states = {}
        for overseer_id in overseer_node_states.keys():
            res = requests.get("http://raspi{:0>2}:5000/state".format(overseer_id)).content
            states[overseer_id] = NodeState(int(res))
        return str(states)


class Timestamps(Resource):
    def get(self):
        return overseer_node_timestamps

class Start(Resource):
    def get(self):
        global stop_syncing
        global start_action_thread
        # add timed event source to check acknowledgments
        try:
            if start_action_thread == None or not start_action_thread.is_alive():
                stop_syncing = False
                start_action_thread = threading.Thread(target=start_overseers)
                start_action_thread.start()
            else:
                return('start job already running')
        except:
            return "Could not start thread"
        return "Starting overseers. Check status at /state"

class Stop(Resource):
    stop_action_thread = None

    def get(self):
        global stop_syncing
        global start_action_thread
        # add timed event source to check acknowledgments
        if start_action_thread != None and start_action_thread.is_alive():
            stop_syncing = True
            start_action_thread.join()

            try:
                if Stop.stop_action_thread == None or not Stop.stop_action_thread.is_alive():
                    Stop.stop_action_thread = threading.Thread(target=stop_overseers)
                    Stop.stop_action_thread.start()
                else:
                    return('stop job already running')

            except:
                return "Could not start thread"
            return "Stopping overseers. Check status at /state"
        else:
            return "No test running"

api.add_resource(Start, '/start')
api.add_resource(Stop, '/stop')
api.add_resource(State, '/state')
api.add_resource(Timestamps, '/timestamps')


if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=False, port=5000)
