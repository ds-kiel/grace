from flask import Flask
from flask_restful import Resource, Api

import dbus

system_bus = dbus.SystemBus()

controller = system_bus.get_object('org.cau.gpiot',
                            '/org/cau/gpiot/Controller')
controller_iface = dbus.Interface(controller,
    dbus_interface='org.cau.gpiot.ControllerInterface')

app = Flask(__name__)
api = Api(app)

class Start(Resource):
    def get(self):
        gpiotd_result = controller_iface.Start("sigrok", 8000000, "/tmp/testing-collector-rest.csv", True) # result :: dbus.String
        return gpiotd_result == "Started collecting on device sigrok" # Big uff. Use dbus error types or something

class Stop(Resource):
    def get(self):
        gpiotd_result = controller_iface.Stop("sigrok", True) # result :: dbus.String
        return gpiotd_result == "Stopped collecting on device sigrok"

class CollectorState(Resource):
    def get(self):
        gpiotd_result = controller_iface.getState() # result :: dbus.String
        return gpiotd_result


api.add_resource(Start, '/start')
api.add_resource(Stop, '/stop')
api.add_resource(CollectorState, '/state')

if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True)
