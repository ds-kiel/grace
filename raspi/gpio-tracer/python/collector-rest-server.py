from flask import Flask
from flask_restful import Resource, Api

import dbus

system_bus = dbus.SystemBus()

controller = bus.get_object('org.cau.gpiot',
                            '/org/cau/gpiot/Controller')
controller_iface = dbus.Interface(controller,
    dbus_interface='org.cau.gpiot.ControllerInterface')

app = Flask(__name__)
api = Api(app)

class HelloWorld(Resource):
    def get(self):
        return {'hello': 'world'}

class AnnounceStart(Resource):
    def get(self):
        gpiotd_result = controller_iface.AnnounceStart() # result :: dbus.String
        return {str(gpiotd_result)}

class AnnounceStop(Resource):
    def get(self):
        gpiotd_result = controller_iface.AnnounceStop() # result :: dbus.String
        return {str(gpiotd_result)}

class CheckState(Resource):
    def get(self):
        gpiotd_result = controller_iface.CheckState() # result :: dbus.String
        return {str(gpiotd_result)}

api.add_resource(HelloWorld, '/')
api.add_resource(AnnounceStart, '/AnnounceStart')

if __name__ == '__main__':
    app.run(debug=True)


