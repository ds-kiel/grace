from flask import Flask, make_response
from flask_restful import Resource, Api

import dbus
import os.path

system_bus = dbus.SystemBus()
controller = system_bus.get_object('org.cau.gpiot',
                            '/org/cau/gpiot/Controller')
controller_iface = dbus.Interface(controller,
    dbus_interface='org.cau.gpiot.ControllerInterface')

app = Flask(__name__)
api = Api(app)

class Start(Resource):
    def get(self):
        gpiotd_result = controller_iface.Start("/tmp/testing-collector-rest.csv", True) # result :: dbus.String
        return gpiotd_result == "Started collecting on device sigrok" # Big uff. Use dbus error types or something

class Stop(Resource):
    def get(self):
        gpiotd_result = controller_iface.Stop(True)
        return gpiotd_result == "Stopped collecting on device sigrok"

class CollectorState(Resource):
    def get(self):
        gpiotd_result = controller_iface.getState()
        return gpiotd_result

class Logs(Resource):
    def get(self):
        if os.path.isfile("/tmp/testing-collector-rest.csv"):
            with open("/tmp/testing-collector-rest.csv", "r") as logfile:
                response = make_response(logfile.read(), 200)
                response.mimetype = "text/plain"
                return response
        return False


api.add_resource(Start, '/start')
api.add_resource(Stop, '/stop')
api.add_resource(Logs, '/log')
api.add_resource(CollectorState, '/state')

if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True)
