#
# Standard version level Makefile used to build a Docker container for MickMake projects.
#


# Determine whether we use "nodeJS" or "JQ".
JSONCMD := $(shell jq -r '.project' ./container.json)
ifneq ($(JSONCMD),)

define GetFromPkg
$(shell jq -r ".$(1)" ./container.json)
endef

else
JSONCMD := $(shell node -p "require('./container.json').project")
ifneq ($(JSONCMD),)
define GetFromPkg
$(shell node -p "require('./container.json').$(1)")
endef

else
$(shell )

endif
endif


# Set global variables from container file.
PROJECT		:= $(call GetFromPkg,project)
NAME		:= $(call GetFromPkg,name)
VERSION		:= $(call GetFromPkg,version)
MAJORVERSION	:= $(call GetFromPkg,majorversion)
LATEST		:= $(call GetFromPkg,latest)
NETWORK		:= $(call GetFromPkg,network)
PORTS		:= $(call GetFromPkg,ports)
VOLUMES		:= $(call GetFromPkg,volumes)
RESTART		:= $(call GetFromPkg,restart)
ARGS		:= $(call GetFromPkg,args)
ENV		:= $(call GetFromPkg,env)

IMAGE_NAME	?= $(PROJECT)/$(NAME)
CONTAINER_NAME	?= $(NAME)-$(VERSION)



.PHONY: build push release reallyclean clean test shell run start stop rm

################################################################################
# Image related commands.
default: build


build: Dockerfile
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	clear; clear; make reallyclean; docker build -t $(IMAGE_NAME):$(VERSION) --build-arg $(ENV)=$(VERSION) .
ifneq ($(MAJORVERSION),)
	-docker build -t $(IMAGE_NAME):$(MAJORVERSION) --build-arg $(ENV)=$(MAJORVERSION) .
endif
ifeq ($(LATEST),1)
	docker build -t $(IMAGE_NAME):latest --build-arg $(ENV)=latest .
endif
endif


push:
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	-docker push $(IMAGE_NAME):$(VERSION)
ifneq ($(MAJORVERSION),)
	-docker push $(IMAGE_NAME):$(MAJORVERSION)
endif
ifeq ($(LATEST),1)
	docker push $(IMAGE_NAME):latest
endif
endif


release: build
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	make build
	make push
endif


reallyclean:
	make rm; make clean


clean:
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	docker image rm $(IMAGE_NAME):$(VERSION)
ifneq ($(MAJORVERSION),)
	docker image rm $(IMAGE_NAME):$(MAJORVERSION)
endif
ifeq ($(LATEST),1)
	docker image rm $(IMAGE_NAME):latest
endif
endif


list:
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	docker image ls $(IMAGE_NAME):$(VERSION)
ifneq ($(MAJORVERSION),)
	docker image ls $(IMAGE_NAME):$(MAJORVERSION)
endif
ifeq ($(LATEST),1)
	docker image ls $(IMAGE_NAME):latest
endif
endif



################################################################################
# Container related commands.

test:
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	make stop
	make rm
	make clean
	make build
	-make create
	-make start
endif

shell:
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	docker run --rm -i -t $(NETWORK) $(PORTS) $(ARGS) $(VOLUMES) $(IMAGE_NAME):$(VERSION) /bin/bash
endif

run:
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	docker run --rm -i -t --name $(CONTAINER_NAME)             $(NETWORK) $(PORTS) $(ARGS) $(VOLUMES) $(IMAGE_NAME):$(VERSION)
endif

create:
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	docker create   --name $(CONTAINER_NAME) $(RESTART)  $(NETWORK) $(PORTS) $(ARGS) $(VOLUMES) $(IMAGE_NAME):$(VERSION)
endif

start:
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	docker start $(CONTAINER_NAME)
endif

stop:
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	docker stop $(CONTAINER_NAME)
endif

rm:
ifeq ($(PROJECT),)
	@echo "MickMake: ERROR - You need to install JQ or NodeJS."
else
	docker container rm $(CONTAINER_NAME)
endif


################################################################################

