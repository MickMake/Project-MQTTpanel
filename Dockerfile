FROM mickmake/rpi-mqtt:latest

MAINTAINER Mick Hellstrom <mick@mickmake.com>

COPY files/ /

RUN apk update && \
	apk add --no-cache python py-paho-mqtt py-pillow libstdc++ make rsync g++ \
		ttf-freefont ttf-opensans ttf-dejavu ttf-inconsolata ttf-ubuntu-font-family ttf-droid ttf-liberation ttf-linux-libertine \
		graphicsmagick-dev zlib-dev freetype-dev mosquitto-dev python-dev json-c-dev python2-dev && \
	cd /root/rpi-rgb-led-matrix && \
	HARDWARE_DESC=adafruit-hat-pwm make && \
	mkdir -p /usr/local/include/rgbmatrix && \
	cp include/* /usr/local/include/rgbmatrix && \
	cp lib/librgbmatrix.* /usr/local/lib && \
	ln -s librgbmatrix.so.1 /usr/local/lib/librgbmatrix.so && \
	HARDWARE_DESC=adafruit-hat-pwm make install-python
	#mkdir -p /usr/local/share/fonts/rgbmatrix && \
	#cp fonts/* /usr/local/share/fonts/rgbmatrix


# ttf-cantarell ttf-font-awesome ttf-mononoki
WORKDIR /root

EXPOSE 1883

CMD ["/usr/local/bin/startup.sh"]
