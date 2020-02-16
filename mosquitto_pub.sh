#!/bin/bash
exec docker run --rm -i -t timmah88/project-mqttpanel:latest /usr/bin/mosquitto_pub -d -h 172.17.0.1 "$@"
