
![MickMake](https://www.mickmake.com/banner.png)


# Project - MQTT RGB matrix panel
This repository contains all the development files, (Python & C), to create your own MQTT based RGB matrix panel. Should take you under an hour to get this up and running from scratch.


## Quick install (for the impatient)
If you don't already have Docker, then install:
`curl -sSL https://get.docker.com | sh`

Then pull the project-mqttpanel Docker image from DockerHub:
`docker pull mickmake/project-mqttpanel:0.10`

Create a Docker container:
`docker create --name project-mqttpanel-0.10 --restart unless-stopped -p 1883:1883 --device /dev/mem:/dev/mem --privileged --user root mickmake/project-mqttpanel:0.10`

Start it up:
docker start project-mqttpanel-0.10

## Sending MQTT messages
If you have the Mosquitto client tools installed somewhere, then you can change the background image using:
`mosquitto_pub -d -h PI_HOSTNAME -t /display/background/load -m HeartBeat.gif`
`mosquitto_pub -d -h PI_HOSTNAME -t /display/background/load -m Hand4.gif`

If you don't, you can use this Docker image to send MQTT messages:
`docker run --rm -i -t mickmake/project-mqttpanel:0.10 /usr/bin/mosquitto_pub -d -h 172.17.0.1 -t /display/background/load -m HeartBeat.gif`

The GitHub repo has some handy shell scripts to stop, start, and publish MQTT messages.

Creating and starting the container:
`./create.sh`

Stopping the container:
`docker stop project-mqttpanel-0.10`
`./stop.sh`

Publish MQTT messages using this container:
`./mosquitto_pub.sh -t /display/background/load -m Alien.gif`


## Supported tags and respective Dockerfiles

`0.10`, `latest` _([0.10/Dockerfile](https://github.com/MickMakes/Project-MQTTpanel/blob/master/Dockerfile))_ 


## Available MQTT messages:
* /display/panel/brightness - Adjust the birghtness of the panel, (0 to 100).
* /display/background/load - Load a different background image from the images directory, (/usr/local/share/rgbmatrix/images).

* /display/time/format - Change the time display format, (uses strftime).
* /display/time/show - Enable or disable time display.
* /display/time/x - Change the 'x' position.
* /display/time/y - Change the 'y' position.
* /display/time/dir - Change the font directory, (shouldn't ned to do this).
* /display/time/font - Font used.
* /display/time/color - Text colour of the time display.

* /display/date/format - Change the date display format, (uses strftime).
* /display/date/show - Enable or disable date display.
* /display/date/x - Change the 'x' position.
* /display/date/y - Change the 'y' position.
* /display/date/dir - Change the font directory, (shouldn't ned to do this).
* /display/date/font - Font used.
* /display/date/color - Text colour of the date display.

* /display/text/format - What additional text you want to display.
* /display/text/show - Enable or disable additional text display.
* /display/text/x - Change the 'x' position.
* /display/text/y - Change the 'y' position.
* /display/text/dir - Change the font directory, (shouldn't ned to do this).
* /display/text/font - Font used.
* /display/text/color - Text colour of additional text.

## Examples:
Display "warning" in red text, change the background image and remove date and time display.
```
mosquitto_pub -h MQTT_pi -t /display/text/format -m 'Warning'
mosquitto_pub -h MQTT_pi -t /display/text/color -m '#FF0000'
mosquitto_pub -h MQTT_pi -t /display/background/load -m 'DancingSkeletons.gif'
mosquitto_pub -h MQTT_pi -t /display/date/show -m 0
mosquitto_pub -h MQTT_pi -t /display/time/show -m 0
```

Display "Congrats" in green text, and change the background image to clapping hands.
```
mosquitto_pub -h MQTT_pi -t /display/text/format -m 'Congrats'
mosquitto_pub -h MQTT_pi -t /display/text/color -m '#00FF00'
mosquitto_pub -h MQTT_pi -t /display/background/load -m 'Hand3.gif'
```


## Using this container.
Two options.
* Pull from Docker Hub.
* Or you can use the GitHub method to build and run the container.
The container ends up being around 250MB. DockerHub should be the quickest method if you don't want to customize, but once pulled you can modify.


## Using it from Docker Hub

### Links
(Docker Hub repo)[https://hub.docker.com/r/mickmake/project-mqttpanel/]

(Docker Cloud repo)[https://cloud.docker.com/swarm/mickmake/repository/docker/mickmake/project-mqttpanel/]


### Setup from Docker Hub
A simple `docker pull mickmake/project-mqttpanel` will pull down the latest version.


### Runtime from Docker Hub
start - Spin up a Docker container with the correct runtime configs. This will later support remote builds, (to avoid restarting containers).

`docker create --name project-mqttpanel --restart unless-stopped -p 1883:1883 --device /dev/mem:/dev/mem --privileged --user root mickmake/project-mqttpanel:0.10`

stop - Stop a Docker container.

`docker stop project-mqttpanel`

run - Run a Docker container in the foreground, (all STDOUT and STDERR will go to console). The Container will be removed on termination.

`docker run --rm mickmake/project-mqttpanel:latest`

shell - Run a shell, (/bin/bash), within a Docker container.

`docker run --rm -i -t mickmake/project-mqttpanel:0.10 /bin/bash`

rm - Remove the Docker container. Not needed if you use the `-i -t` flags for shells.

`docker container rm project-mqttpanel`


## Using it from GitHub repo

### Setup from GitHub repo
Simply clone this repository to your local machine

`git clone https://github.com/mickmakes/Project-MQTTpanel.git`


### Building from GitHub repo

`make build` - Build Docker images. Build all versions from the base directory or specific versions from each directory.


`make list` - List already built Docker images. List all versions from the base directory or specific versions from each directory.


`make clean` - Remove already built Docker images. Remove all versions from the base directory or specific versions from each directory.


`make push` - Push already built Docker images to Docker Hub, (only for MickMake admins). Push all versions from the base directory or specific versions from each directory.


### Runtime from GitHub repo
When you `cd` into a version directory you can also perform a few more actions.

`make start` - Spin up a Docker container with the correct runtime configs.


`make stop` - Stop a Docker container.


`make run` - Run a Docker container in the foreground, (all STDOUT and STDERR will go to console). The Container be removed on termination.


`make shell` - Run a shell, (/bin/bash), within a Docker container.


`make rm` - Remove the Docker container.


`make test` - Will issue a `stop`, `rm`, `clean`, `build`, `create` and `start` on a Docker container.


