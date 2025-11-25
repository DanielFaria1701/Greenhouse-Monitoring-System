import time
import paho.mqtt.client as mqtt
import ssl
import sys
import random

# Parameters:
CLOUD_MQTT_URL = "a64v0onp3m97o-ats.iot.eu-west-1.amazonaws.com"
CERTIFICATE_AUTH_FILE = "Projeto_Cert/AmazonRootCA1.pem"
CERT_PEM_FILE = "Projeto_Cert/3991493ada418cfe69e1f792564881f3c86d129aeefe48655517a8022c95a242-certificate.pem.crt"
PRIVATE_KEY_FILE = "Projeto_Cert/3991493ada418cfe69e1f792564881f3c86d129aeefe48655517a8022c95a242-private.pem.key"
MQTT_TOPIC = "projeto/sensor"

#Override MQTT_TOPIC from the cmd line:
if len(sys.argv) > 1:
  MQTT_TOPIC = str(sys.argv[1])


client=mqtt.Client() 

print("Connecting to Cloud MQTT Broker")
client.tls_set(ca_certs=CERTIFICATE_AUTH_FILE, certfile=CERT_PEM_FILE, keyfile=PRIVATE_KEY_FILE, tls_version=ssl.PROTOCOL_TLSv1_2)
client.tls_insecure_set(False)
client.connect(CLOUD_MQTT_URL, 8883, 60)

##start loop to process received messages
#client.loop_start()
id_count = 0
ts_count = 1555980655000
print("Setup a publisher in topic: \""+MQTT_TOPIC+"\"")

while True:
   try: 
          temp = random.uniform(0, 30)
          humidity = random.uniform(0, 100)
          light = random.uniform(0, 1000)
          msg="{\n\"id\": \"" + str(id_count) + "\",\n\"ts\": \"" + str(ts_count) + "\",\n\"temp\": \"" + str(temp) + "\",\n\"humidity\": \"" + str(humidity) + "\",\n\"light\": \"" + str(light) + "\"\n}"
          print("publishing: " + msg)
     
          client.publish(MQTT_TOPIC,msg)
          id_count+=1
          ts_count+=1000000000
          #wait to allow publishing continuously
          time.sleep(10)
   except (KeyboardInterrupt):
        sys.exit()


