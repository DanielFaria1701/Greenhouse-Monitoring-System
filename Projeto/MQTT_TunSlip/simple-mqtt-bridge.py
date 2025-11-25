# This "bridge" script subscribes to messages inside the 6LoWPAN network using unencrypted MQTT
# and then publishes the messages to an external MQTT broker

# You can easily change this script, in case you need to subscribe to external topics 
# and publish them locally in the 6LoWPAN network.

# Libraries:
import paho.mqtt.client as mqtt
import ssl
import sys


############ Please change these parameters for your own setup #########

LOCAL_MQTT_URL = "localhost"

# A topic to publish information from the zolertia board(s):	 
SENSORS_TOPIC = "deec/evt/status"

# Address of the External MQTT Broker:
EXTERNAL_MQTT_URL = "193.136.230.52"

# A topic to subscribe information from the external world:	 
EXTERNAL_TOPIC = "deec/cmd/leds"

#########################################################################

# Callback for initial local network connection:
def on_connect(local_client, userdata, flags, rc):
    print("Connected to local MQTT broker with result code " + str(rc))
    local_client.subscribe(SENSORS_TOPIC)
    print("Subscribed to local topic: " + SENSORS_TOPIC + "\n")

# Callback for received message in the local network:
def on_local_message(local_client, userdata, msg):
    #publish the exact same message on the external MQTT broker:
    print("Local -> External: Topic [" + msg.topic + "]. Msg \""+str(msg.payload)+"\"")
    ext_client.publish(msg.topic,str(msg.payload))

# Callback for received message externally:
def on_ext_message(ext_client, userdata, msg):
    #publish the exact same message on the local MQTT broker:
    print("External -> Local: Topic [" + msg.topic + "]. Msg \""+str(msg.payload)+"\"")
    local_client.publish(msg.topic,str(msg.payload))

#########################################################################

# 1st connect to the External MQTT Client:
ext_client=mqtt.Client() 
ext_client.on_message = on_ext_message

print("Connecting to the External MQTT Broker at " + EXTERNAL_MQTT_URL + "...")
ext_client.connect(EXTERNAL_MQTT_URL, 1883, 60)
print("Connected to the External MQTT Broker.")
ext_client.subscribe(EXTERNAL_TOPIC)
print("Subscribed to external topic: " + EXTERNAL_TOPIC + "\n")

# Then connect to the Local MQTT Client:
local_client = mqtt.Client()
local_client.on_connect = on_connect
local_client.on_message = on_local_message

print("Connecting locally to " + LOCAL_MQTT_URL + "...")
local_client.connect(LOCAL_MQTT_URL, 1883, 60)

# If we had just 1 connection we coud use client.loop_forever()
# since we have two, we'll use a while loop this way:

try:
   while True:
        local_client.loop(0.01) # timeout of 0.01 secs (max 100Hz)
        ext_client.loop(0.01)

except (KeyboardInterrupt): #catch keyboard interrupts
        sys.exit()


# You can test by running on your VM:
# mosquitto_pub -h localhost -t deec/send -m "Publiquei esta msg localmente e a bridge fez forward para fora!"

# Also test publishing something externally and check if you receive it locally with:
# mosquitto_sub -h localhost -t deec/receive


