#! /usr/bin/python3
import time
import sqlite3, json
import paho.mqtt.client as mqtt


mqtt_topic = "sensor/door/pir"
mqtt_username = "janedoe"
mqtt_password = "johndoe"
dbFile = "pir.db"
mqtt_broker_ip = '192.168.1.50'

def takeTime():
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe(mqtt_topic, 0)
# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    time_ = takeTime()
    topic = msg.topic
    payload = json.dumps(msg.payload.decode('utf-8'))
    sql_cmd = sql_cmd = "INSERT INTO {1[sensor]} VALUES ('{0}', {1[motion]})".format(time_, payload)
    writeToDB(sql_cmd)
    print(sql_cmd)
    return None

def writeToDB(sql_command):
    conn = sqlite3.connect(dbFile)
    cur = conn.cursor()
    cur.execute(sql_command)
    conn.commit()

client = mqtt.Client()
client.username_pw_set(username=mqtt_username, password=mqtt_password)                                                                                                                   
client.connect(mqtt_broker_ip, 1883, 60)
client.on_connect = on_connect
client.on_message = on_message
time.sleep(1)

client.loop_forever()
