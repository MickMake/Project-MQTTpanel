#!/bin/bash

ctrlc()
{
	killall mosquitto
}

trap ctrlc INT

/usr/sbin/mosquitto -d

# Check and override the default config.json
if [ -r /tmp/override/config.json ]
then
	diff /tmp/override/config.json /usr/local/etc/config.json >&/dev/null
	if [ "$?" == "1" ]
	then
		echo "Updating config.json"
		cp /tmp/override/config.json /usr/local/etc/config.json
	fi
fi

/usr/local/bin/MQTTpanel
killall mosquitto

