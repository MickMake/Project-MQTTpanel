CXXFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter
OBJECTS=MQTTpanel.o
BINARIES=MQTTpanel

OPTIONAL_OBJECTS=video-viewer.o
OPTIONAL_BINARIES=video-viewer

# Where our library resides. You mostly only need to change the
# RGB_LIB_DISTRIBUTION, this is where the library is checked out.
JSON_INCDIR=/usr/include/json-c
JSON_LIBDIR=/usr/lib
JSON_LIBRARY_NAME=json-c
RGB_INCDIR=/usr/local/include/rgbmatrix
RGB_LIBDIR=/usr/local/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -L$(JSON_LIBDIR) -l$(JSON_LIBRARY_NAME) -lrt -lm -lpthread -lmosquitto

# Imagemagic flags, only needed if actually compiled.
MAGICK_CXXFLAGS=`GraphicsMagick++-config --cppflags --cxxflags`
MAGICK_LDFLAGS=`GraphicsMagick++-config --ldflags --libs`
AV_CXXFLAGS=`pkg-config --cflags  libavcodec libavformat libswscale libavutil`

all : $(BINARIES)

MQTTpanel: MQTTpanel.o $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) MQTTpanel.o -o $@ $(LDFLAGS) $(MAGICK_LDFLAGS)

video-viewer: video-viewer.o $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) video-viewer.o -o $@ $(LDFLAGS) `pkg-config --cflags --libs  libavcodec libavformat libswscale libavutil`

%.o : %.cc
	$(CXX) -I$(RGB_INCDIR) -I$(JSON_INCDIR) $(CXXFLAGS) -c -o $@ $<

MQTTpanel.o : MQTTpanel.cc
	$(CXX) -I$(RGB_INCDIR) -I$(JSON_INCDIR) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -c -o $@ $<

# We're using a couple of deprecated functions. Pull request to update this to
# the latest libraries is welcome.
video-viewer.o: video-viewer.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -Wno-deprecated-declarations -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(BINARIES) $(OPTIONAL_OBJECTS) $(OPTIONAL_BINARIES)

FORCE:
.PHONY: FORCE
