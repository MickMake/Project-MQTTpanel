#!/bin/bash

ctrlc()
{
	killall mosquitto
}

trap ctrlc INT

/usr/sbin/mosquitto -d
/usr/local/bin/MQTTpanel
killall mosquitto

