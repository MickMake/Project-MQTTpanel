#!/usr/bin/python

import json
import paho.mqtt.client as paho
from PIL import Image
from PIL import ImageDraw
from PIL import ImageFont
import time
from rgbmatrix import RGBMatrix
#from samplebase import SampleBase
from rgbmatrix import graphics
from rgbmatrix import RGBMatrix, RGBMatrixOptions


options = RGBMatrixOptions()
options.hardware_mapping = "adafruit-hat"
options.led_rgb_sequence = "RBG"
options.rows = 64
options.cols = 64
options.chain_length = 1
options.disable_hardware_pulsing = True
matrix = RGBMatrix(options = options)

#canvas = matrix
#canvas2 = matrix.CreateFrameCanvas() 

image = Image.new("RGB", (64, 64))  # Can be larger than matrix if wanted!!
draw = ImageDraw.Draw(image)  # Declare Draw instance before prims 

background = (0, 0, 0)

# FreeMono.ttf
# FreeMonoBold.ttf
# FreeMonoBoldOblique.ttf
# FreeMonoOblique.ttf
# FreeSans.ttf
# FreeSansBold.ttf
# FreeSansBoldOblique.ttf
# FreeSansOblique.ttf
# FreeSerif.ttf
# FreeSerifBold.ttf
# FreeSerifBoldItalic.ttf
# FreeSerifItalic.ttf
# LinBiolinum_Kah.ttf
# LinBiolinum_RBah.ttf
# LinBiolinum_RIah.ttf
# LinBiolinum_Rah.ttf
# LinLibertine_DRah.ttf
# LinLibertine_I.ttf
# LinLibertine_Mah.ttf
# LinLibertine_RBIah.ttf
# LinLibertine_RBah.ttf
# LinLibertine_RIah.ttf
# LinLibertine_RZIah.ttf
# LinLibertine_RZah.ttf
# LinLibertine_Rah.ttf
# OpenSans-Bold.ttf
# OpenSans-BoldItalic.ttf
# OpenSans-ExtraBold.ttf
# OpenSans-ExtraBoldItalic.ttf
# OpenSans-Italic.ttf
# OpenSans-Light.ttf
# OpenSans-LightItalic.ttf
# OpenSans-Regular.ttf
# OpenSans-Semibold.ttf
# OpenSans-SemiboldItalic.ttf

#fonts = []
#
#print '# Loading fonts'
#for f in font_list:
#	print '    #', f[0], f[1]
#	fonts.append(ImageFont.truetype(f[0], f[1]))
#print '# loaded'

FontList = {
        'small': {'font':'FreeMono.ttf', 'size':8},
        'default': {'font':'FreeMono.ttf', 'size':20},
        }

ColourList = {
        'blue': '0000FF',
        'green': '00FF00',
        'red': 'FF0000',
        'white': 'FFFFFF',
        'black': '000000',
        }



def runTime():
    global matrix
    offscreen_canvas = matrix.CreateFrameCanvas()

    # Figure out the font we are using.
    json_font = FontList['default'].get('font')
    json_size = FontList['default'].get('size')
    font = graphics.Font()
    font.LoadFont("/usr/share/fonts/TTF/" + json_font)
    #font.LoadFont("/root/rpi-rgb-led-matrix/fonts/5x8.bdf")
    textColor = graphics.Color(255, 255, 0)
    pos = offscreen_canvas.width
    my_text = 'foo'
    previousTime = ''
    print("/usr/share/fonts/TTF/" + json_font)

    while True:
        offscreen_canvas.Clear()
        len = graphics.DrawText(offscreen_canvas, font, pos, 10, textColor, my_text)
        pos -= 1
        if (pos + len < 0):
            pos = offscreen_canvas.width
        print pos
        time.sleep(0.05)
        offscreen_canvas = matrix.SwapOnVSync(offscreen_canvas)


def on_message(clnt, userdata, msg):
    global background
    global matrix
    print msg.payload

    j = json.loads(str(msg.payload))

    if 'cmd' in j:
        if j['cmd'] == 'text':
            # Figure out the font we are using.
            wantFont = j.get('font', 'default')
            json_font = FontList[wantFont].get('font')
            json_size = FontList[wantFont].get('size')

            # Fetch colour of text. 
            wantColour = j.get('colour', '#FFFFFF').lstrip('#')
            try:
                json_colour = tuple(int(wantColour[i:i+2], 16) for i in (0, 2 ,4))
            except ValueError:
                wantColour = ColourList.get(j.get('colour'), 'FFFFFF')
                json_colour = tuple(int(wantColour[i:i+2], 16) for i in (0, 2 ,4))

            # Positioning.
            json_x = int(j.get('x', "0"))
            json_y = int(j.get('y', "0"))

            json_text = j.get('text', '')

            canvas = matrix.CreateFrameCanvas()
            font = ImageFont.truetype(json_font, json_size)

            text_w, text_h = draw.textsize(json_text, font=font)
            draw.rectangle((json_x, json_y, json_x+text_w, json_y+text_h), fill=background)

            draw.text((json_x, json_y), json_text, font=font, fill=(json_colour[0], json_colour[1], json_colour[2]))
            canvas.SetImage(image, 0, 0)

            canvas = matrix.SwapOnVSync(canvas)

        # Display the time.
        if j['cmd'] == 'time':
            # Figure out the font we are using.
            wantFont = j.get('font', 'default')
            json_font = FontList[wantFont].get('font')
            json_size = FontList[wantFont].get('size')

            # Fetch colour of text. 
            wantColour = j.get('colour', '#FFFFFF').lstrip('#')
            try:
                json_colour = tuple(int(wantColour[i:i+2], 16) for i in (0, 2 ,4))
            except ValueError:
                wantColour = ColourList.get(j.get('colour'), 'FFFFFF')
                json_colour = tuple(int(wantColour[i:i+2], 16) for i in (0, 2 ,4))

            # Positioning.
            json_x = int(j.get('x', "0"))
            json_y = int(j.get('y', "0"))

            json_text = j.get('text', '')

            canvas = matrix.CreateFrameCanvas()
            font = ImageFont.truetype(json_font, json_size)

            text_w, text_h = draw.textsize(json_text, font=font)
            draw.rectangle((json_x, json_y, json_x+text_w, json_y+text_h), fill=background)

            draw.text((json_x, json_y), json_text, font=font, fill=(json_colour[0], json_colour[1], json_colour[2]))
            canvas.SetImage(image, 0, 0)

            canvas = matrix.SwapOnVSync(canvas)


        if j['cmd'] == 'clear':
            print '# clear'
            canvas = matrix.CreateFrameCanvas()

            json_red = int(j['red'])
            json_green = int(j['green'])
            json_blue = int(j['blue'])

            background = (json_red, json_green, json_blue)
            draw.rectangle((0, 0, matrix.width, matrix.height), fill=background)
            canvas.SetImage(image, 0, 0)

            canvas = matrix.SwapOnVSync(canvas)


mqtt = paho.Client()
mqtt.on_message = on_message
# mqtt.username_pw_set("admin", "admin") # credentials from Node-RED installation
mqtt.connect("172.17.0.1", 1883)
mqtt.subscribe("panel/1")
print '# Ready'
#mqtt.loop_forever()
runTime()
while 1:
    mqtt.loop(.1)



