import paho.mqtt.client as mqtt
from flask import Flask, render_template, request
from flask_socketio import SocketIO, emit

app = Flask(__name__)
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app)


def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("/esp8266/temperature")
    client.subscribe("/esp8266/humidity")
    client.subscribe("/esp8266/moisture")


def on_message(client, userdata, message):
    #socketio.emit('my variable')
    print("Received message '" + str(message.payload) + "' on topic '"
        + message.topic + "' with QoS " + str(message.qos))
    if message.topic == "/esp8266/temperature":
        print("temperature update")
	socketio.emit('dht_temperature', {'data': message.payload})
    if message.topic == "/esp8266/humidity":
        print("humidity update")
	socketio.emit('dht_humidity', {'data': message.payload})
    if message.topic == "/esp8266/moisture":
        print("moisture update")
	socketio.emit('moisture', {'data': message.payload})

mqttc=mqtt.Client()
mqttc.on_connect = on_connect
mqttc.on_message = on_message
mqttc.connect("localhost",1883,60)
mqttc.loop_start()

# Create a dictionary called pins to store the pin number, name, and pin state:
pins = {
   4 : {'name' : 'GPIO 4', 'board' : 'esp8266', 'topic' : 'esp8266/4', 'state' : 'False'},
   5 : {'name' : 'GPIO 5', 'board' : 'esp8266', 'topic' : 'esp8266/5', 'state' : 'False'}
   }

templateData = {
   'pins' : pins
   }

@app.route("/")
def main():
   return render_template('main.html', async_mode=socketio.async_mode, **templateData)

@app.route("/<board>/<changePin>/<action>")
def action(board, changePin, action):
   changePin = int(changePin)
   devicePin = pins[changePin]['name']
   if action == "1" and board == 'esp8266':
      mqttc.publish(pins[changePin]['topic'],"1")
      pins[changePin]['state'] = 'True'
   if action == "0" and board == 'esp8266':
      mqttc.publish(pins[changePin]['topic'],"0")
      pins[changePin]['state'] = 'False'
   templateData = {
      'pins' : pins
   }
   return render_template('main.html', **templateData)

@socketio.on('my event')
def handle_my_custom_event(json):
    print('received json data here: ' + str(json))

if __name__ == "__main__":
   socketio.run(app, host='0.0.0.0', port=8181, debug=True)