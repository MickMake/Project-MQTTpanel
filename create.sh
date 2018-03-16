#!/bin/bash
docker create --name project-mqttpanel --restart unless-stopped -p 1883:1883 --device /dev/mem:/dev/mem --privileged --user root mickmake/project-mqttpanel:latest
docker start project-mqttpanel
