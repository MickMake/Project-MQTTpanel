#!/bin/bash
exec docker run --rm -i -t mickmake/project-mqttpanel:latest /usr/bin/mosquitto_pub -d -h 172.17.0.1 "$@"
