extern "C"
{
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <err.h>
#include <dirent.h>
#include <time.h>
}

#include <algorithm>
#include <map>
#include <string>
//#include <iostream>
#include <vector>

#include <Magick++.h>
#include <magick/image.h>

#include "led-matrix.h"
#include "graphics.h"
#include "transformer.h"
#include "content-streamer.h"
#include "json.h"
#include "json_util.h"
#include "mosquitto.h"
#include "led-matrix.h"

#define mqtt_host "localhost"
#define mqtt_port 1883

using rgb_matrix::GPIO;
using rgb_matrix::Canvas;
using rgb_matrix::Color;
using rgb_matrix::FrameCanvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::StreamReader;
int sflags = 0;
#define json_object_to_json_string(obj) json_object_to_json_string_ext(obj,sflags)

typedef int64_t tmillis_t;
static const tmillis_t distant_future = (1LL<<40); // that is a while.

struct FileInfo
{
	// ImageParams params;      // Each file might have specific timing settings
	bool is_multi_frame;
	rgb_matrix::StreamIO *content_stream;
};
rgb_matrix::StreamIO *content_stream;

struct ImageParams
{
	ImageParams() : anim_duration_ms(distant_future), wait_ms(1500), anim_delay_ms(-1), loops(1), is_multi_frame(1), content_stream(NULL) {}
	tmillis_t anim_duration_ms;  // If this is an animation, duration to show.
	tmillis_t wait_ms;           // Regular image: duration to show.
	tmillis_t anim_delay_ms;     // Animation delay override.
	int loops;
	bool is_multi_frame;
	rgb_matrix::StreamIO *content_stream;
};

struct _displayImage
{
	const char *dir;
	const char *image;
	uint8_t x;
	uint8_t y;
	bool show;
} displayImage;

struct _Text
{
	const char *format;
	const char *dir;
	const char *fontname;
	rgb_matrix::Font ptrFont;
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t x;
	uint8_t y;
	bool show;
};

_Text displayTime;
_Text displayDate;
_Text displayText;

json_object *ConfigJSON;
std::map<const void *, struct ImageParams> filename_params;
std::vector<FileInfo*> file_imgs;
ImageParams bgImage;
RGBMatrix *canvas;
FrameCanvas *offscreen_canvas;
struct mosquitto *mosq;
RGBMatrix::Options panelOptions;
rgb_matrix::StreamWriter *global_stream_writer = NULL;

volatile bool Interrupt = false;
int vsync_multiple = 1;
bool do_forever = false;
bool do_center = false;
bool do_shuffle = false;
//bool large_display = false;  // 64x64 made out of 4 in sequence.
int angle = -361;

static int usage(const char *progname);
static tmillis_t GetTimeInMillis();
static bool LoadImageAndScale(const char *filename, int target_width, int target_height, bool fill_width, bool fill_height, std::vector<Magick::Image> *result, std::string *err_msg);
void LoadImage(const char *filename);
static void StoreInStream(const Magick::Image &img, int delay_time_us, bool do_center, rgb_matrix::FrameCanvas *scratch, rgb_matrix::StreamWriter *output);
void DisplayAnimation(RGBMatrix *matrix, FrameCanvas *offscreen_canvas, int vsync_multiple);
int initMQTT(void);
int runMQTT(void);
int initPanel(int argc, char *argv[]);
void connect_callback(struct mosquitto *mosq, void *obj, int result);
void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);
static void InterruptHandler(int signo);
void setPanelConfig(char *Key);
void setPanelConfig(char *Key, char *Value, bool JSONpref);
void setPanelConfig(char *Key, int Value, bool JSONpref);
void setPanelConfig(char *Key, bool Value, bool JSONpref);
bool fetchFont(_Text *Text);
bool convRGBstr(char *str, uint8_t *red, uint8_t *green, uint8_t *blue);


// ################################################################################
int main(int argc, char *argv[])
{
	signal(SIGTERM, InterruptHandler);
	signal(SIGINT, InterruptHandler);

	const char *filename = "/usr/local/etc/config.json";
	int fileJSON = open(filename, O_RDONLY, 0);
	if (fileJSON < 0)
	{
		fprintf(stderr, "FAIL: unable to open %s: %s\n", filename, strerror(errno));
		exit(0);
	}
	close(fileJSON);

	printf("Reading JSON config file '%s'...", filename);
	ConfigJSON = json_object_from_file(filename);
	if (ConfigJSON != NULL)
	{
		printf("Done\n");
		//printf("OK: json_object_from_fd(%s)=%s\n", filename, json_object_to_json_string(ConfigJSON));
	}
	else
	{
		fprintf(stderr, "FAIL: unable to parse contents of %s: %s\n", filename, json_object_to_json_string(ConfigJSON));
		exit(1);
	}

	initPanel(argc, argv);
	initMQTT();

	while(!Interrupt)
	{
		DisplayAnimation(canvas, offscreen_canvas, vsync_multiple);
		runMQTT();
	}

	mosquitto_lib_cleanup();

	// Animation finished. Shut down the RGB canvas.
	canvas->Clear();
	delete canvas;

	if (Interrupt)
	{
		fprintf(stderr, "Caught signal. Exiting.\n");
	}
	printf("\nEnded.\n\n");

	return 0;
}



// ################################################################################
// GIF drawing.
static void InterruptHandler(int signo)
{
	Interrupt = true;
}


/*
static void ShowWorm(Canvas *canvas)
{
	//
	//Let's create a simple animation. We use the canvas to draw
	//pixels. We wait between each step to have a slower animation.
	//
	canvas->Fill(0, 0, 255);

	int center_x = canvas->width() / 2;
	int center_y = canvas->height() / 2;
	float radius_max = canvas->width() / 2;
	float angle_step = 1.0 / 360;
	for (float a = 0, r = 0; r < radius_max; a += angle_step, r += angle_step)
	{
		if (Interrupt)
			return;
		float dot_x = cos(a * 2 * M_PI) * r;
		float dot_y = sin(a * 2 * M_PI) * r;
		canvas->SetPixel(center_x + dot_x, center_y + dot_y, 255, 0, 0);
		usleep(1 * 1000);  // wait a little to slow down things.
	}
}
*/


bool convRGBstr(char *str, uint8_t *red, uint8_t *green, uint8_t *blue)
{
	unsigned int redInt;
	unsigned int greenInt;
	unsigned int blueInt;
	bool OK = false;

	if (sscanf(str, "#%02x%02x%02x", &redInt, &greenInt, &blueInt) == 3)
		OK = true;
	else if (sscanf(str, "%02x%02x%02x", &redInt, &greenInt, &blueInt) == 3)
		OK = true;
	else if (sscanf(str, "0x%02x%02x%02x", &redInt, &greenInt, &blueInt) == 3)
		OK = true;

	if (!OK)
		return(false);

	(*red) = (uint8_t)(redInt & 0xFF);
	(*green) = (uint8_t)(greenInt & 0xFF);
	(*blue) = (uint8_t)(blueInt & 0xFF);
	return(true);
}


void setPanelConfig(char *Key)
{
	if (strcmp(Key, "show_refresh_rate") == 0)
		setPanelConfig(Key, true, true);

	else if (strcmp(Key, "inverse_colors") == 0)
		setPanelConfig(Key, true, true);

	else if (strcmp(Key, "hardware_mapping") == 0)
		setPanelConfig(Key, (char *)NULL, true);

	else if (strcmp(Key, "led_rgb_sequence") == 0)
		setPanelConfig(Key, (char *)NULL, true);

	else if (strcmp(Key, "rows") == 0)
		setPanelConfig(Key, (int)0, true);

	else if (strcmp(Key, "cols") == 0)
		setPanelConfig(Key, (int)0, true);

	else if (strcmp(Key, "chain_length") == 0)
		setPanelConfig(Key, (int)0, true);

	else if (strcmp(Key, "parallel") == 0)
		setPanelConfig(Key, (int)0, true);

	else if (strcmp(Key, "multiplexing") == 0)
		setPanelConfig(Key, (int)0, true);

	else if (strcmp(Key, "brightness") == 0)
		setPanelConfig(Key, (int)0, true);

	else if (strcmp(Key, "scan_mode") == 0)
		setPanelConfig(Key, (int)0, true);

	else if (strcmp(Key, "pwm_bits") == 0)
		setPanelConfig(Key, (int)0, true);

	else if (strcmp(Key, "pwm_lsb_nanoseconds") == 0)
		setPanelConfig(Key, (int)0, true);

	else if (strcmp(Key, "row_address_type") == 0)
		setPanelConfig(Key, (int)0, true);

}


void setPanelConfig(char *Key, bool Value, bool JSONpref)
{
	struct json_object *rootObject;
	if (!json_object_object_get_ex(ConfigJSON, "panel", &rootObject))
		return;


	bool *ptrKey;
	if (strcmp(Key, "show_refresh_rate") == 0)
		ptrKey = &panelOptions.show_refresh_rate;

	else if (strcmp(Key, "inverse_colors") == 0)
		ptrKey = &panelOptions.inverse_colors;

	else
		return;


	struct json_object *keyObject;
	if (!json_object_object_get_ex(rootObject, Key, &keyObject))
	{
		keyObject = json_object_new_boolean((json_bool)(*ptrKey));
		json_object_object_add(rootObject, Key, keyObject);
	}
	if (JSONpref == true)
		(*ptrKey) = (bool *) json_object_get_boolean(keyObject);
	else
		(*ptrKey) = Value;
}


void setPanelConfig(char *Key, char *Value, bool JSONpref)
{
	struct json_object *rootObject;
	if (!json_object_object_get_ex(ConfigJSON, "panel", &rootObject))
		return;


	char *ptrKey;
	if (strcmp(Key, "hardware_mapping") == 0)
		ptrKey = (char *)panelOptions.hardware_mapping;

	else if (strcmp(Key, "led_rgb_sequence") == 0)
		ptrKey = (char *)panelOptions.led_rgb_sequence;

	else
		return;


	struct json_object *keyObject;
	if (!json_object_object_get_ex(rootObject, Key, &keyObject))
	{
		keyObject = json_object_new_string(ptrKey);
		json_object_object_add(rootObject, Key, keyObject);
	}
	if (JSONpref == true)
		strcpy((char *)json_object_get_string(keyObject), ptrKey);
	else
		strcpy(Value, ptrKey);
}


void setPanelConfig(char *Key, int Value, bool JSONpref)
{
	struct json_object *rootObject;
	if (!json_object_object_get_ex(ConfigJSON, "panel", &rootObject))
		return;


	int *ptrKey;
	if (strcmp(Key, "rows") == 0)
		ptrKey = &panelOptions.rows;

	else if (strcmp(Key, "cols") == 0)
		ptrKey = &panelOptions.cols;

	else if (strcmp(Key, "chain_length") == 0)
		ptrKey = &panelOptions.chain_length;

	else if (strcmp(Key, "parallel") == 0)
		ptrKey = &panelOptions.parallel;

	else if (strcmp(Key, "multiplexing") == 0)
		ptrKey = &panelOptions.multiplexing;

	else if (strcmp(Key, "brightness") == 0)
		ptrKey = &panelOptions.brightness;

	else if (strcmp(Key, "scan_mode") == 0)
		ptrKey = &panelOptions.scan_mode;

	else if (strcmp(Key, "pwm_bits") == 0)
		ptrKey = &panelOptions.pwm_bits;

	else if (strcmp(Key, "pwm_lsb_nanoseconds") == 0)
		ptrKey = &panelOptions.pwm_lsb_nanoseconds;

	else if (strcmp(Key, "row_address_type") == 0)
		ptrKey = &panelOptions.row_address_type;
	else
		return;


	struct json_object *keyObject;
	if (!json_object_object_get_ex(rootObject, Key, &keyObject))
	{
		keyObject = json_object_new_int((int32_t)(*ptrKey));
		json_object_object_add(rootObject, Key, keyObject);
	}
	if (JSONpref == true)
		(*ptrKey) = json_object_get_int(keyObject);
	else
		(*ptrKey) = Value;
}


// ################################################################################
int initPanel(int argc, char *argv[])
{
	setPanelConfig((char *) "hardware_mapping");
	setPanelConfig((char *) "led_rgb_sequence");
	setPanelConfig((char *) "rows");
	setPanelConfig((char *) "cols");
	setPanelConfig((char *) "chain_length");
	setPanelConfig((char *) "parallel");
	setPanelConfig((char *) "multiplexing");
	setPanelConfig((char *) "brightness");
	setPanelConfig((char *) "scan_mode");
	setPanelConfig((char *) "pwm_bits");
	setPanelConfig((char *) "pwm_lsb_nanoseconds");
	setPanelConfig((char *) "row_address_type");
	setPanelConfig((char *) "show_refresh_rate");
	setPanelConfig((char *) "inverse_colors");
	// printf("JSON:%s\n", json_object_to_json_string(rootObject));

	struct json_object *rootObject;
	if (json_object_object_get_ex(ConfigJSON, "background", &rootObject))
	{
		struct json_object *keyObject;

		if (!json_object_object_get_ex(rootObject, "image", &keyObject))
		{
			keyObject = json_object_new_string("Alien.gif");
			json_object_object_add(rootObject, "image", keyObject);
		}
		displayImage.image = json_object_get_string(keyObject);

		if (!json_object_object_get_ex(rootObject, "dir", &keyObject))
		{
			keyObject = json_object_new_string("images/");
			json_object_object_add(rootObject, "dir", keyObject);
		}
		displayImage.dir = json_object_get_string(keyObject);

		if (!json_object_object_get_ex(rootObject, "show", &keyObject))
		{
			keyObject = json_object_new_boolean(1);
			json_object_object_add(rootObject, "show", keyObject);
		}
		displayImage.show = json_object_get_boolean(keyObject);
	}
	// printf("JSON(background):%s\n", json_object_get_string(rootObject));

	if (json_object_object_get_ex(ConfigJSON, "time", &rootObject))
	{
		struct json_object *keyObject;

		if (!json_object_object_get_ex(rootObject, "format", &keyObject))
		{
			keyObject = json_object_new_string("%H:%M");
			json_object_object_add(rootObject, "format", keyObject);
		}
		displayTime.format = json_object_get_string(keyObject);

		if (!json_object_object_get_ex(rootObject, "dir", &keyObject))
		{
			keyObject = json_object_new_string("/usr/local/share/fonts/rgbmatrix/");
			json_object_object_add(rootObject, "dir", keyObject);
		}
		displayTime.dir = json_object_get_string(keyObject);

		if (!json_object_object_get_ex(rootObject, "font", &keyObject))
		{
			keyObject = json_object_new_string("8x13.bdf");
			json_object_object_add(rootObject, "font", keyObject);
		}
		displayTime.fontname = json_object_get_string(keyObject);
		fetchFont(&displayTime);

		if (!json_object_object_get_ex(rootObject, "x", &keyObject))
		{
			keyObject = json_object_new_int(8);
			json_object_object_add(rootObject, "x", keyObject);
		}
		displayTime.x = json_object_get_int(keyObject);

		if (!json_object_object_get_ex(rootObject, "y", &keyObject))
		{
			keyObject = json_object_new_int(10);
			json_object_object_add(rootObject, "y", keyObject);
		}
		displayTime.y = json_object_get_int(keyObject);

		if (!json_object_object_get_ex(rootObject, "show", &keyObject))
		{
			keyObject = json_object_new_boolean(1);
			json_object_object_add(rootObject, "show", keyObject);
		}
		displayTime.show = json_object_get_boolean(keyObject);

		if (!json_object_object_get_ex(rootObject, "color", &keyObject))
		{
			keyObject = json_object_new_string("FF00FF");
			json_object_object_add(rootObject, "color", keyObject);
		}
		convRGBstr((char *)json_object_get_string(keyObject), &displayTime.red, &displayTime.green, &displayTime.blue);
	}
	// printf("JSON(time):%s\n", json_object_get_string(rootObject));

	if (json_object_object_get_ex(ConfigJSON, "date", &rootObject))
	{
		struct json_object *keyObject;

		if (!json_object_object_get_ex(rootObject, "format", &keyObject))
		{
			keyObject = json_object_new_string("%Y/%m/%d");
			json_object_object_add(rootObject, "format", keyObject);
		}
		displayDate.format = json_object_get_string(keyObject);

		if (!json_object_object_get_ex(rootObject, "dir", &keyObject))
		{
			keyObject = json_object_new_string("/usr/local/share/fonts/rgbmatrix/");
			json_object_object_add(rootObject, "dir", keyObject);
		}
		displayDate.dir = json_object_get_string(keyObject);

		if (!json_object_object_get_ex(rootObject, "font", &keyObject))
		{
			keyObject = json_object_new_string("6x9.bdf");
			json_object_object_add(rootObject, "font", keyObject);
		}
		displayDate.fontname = json_object_get_string(keyObject);
		fetchFont(&displayDate);

		if (!json_object_object_get_ex(rootObject, "x", &keyObject))
		{
			keyObject = json_object_new_int(8);
			json_object_object_add(rootObject, "x", keyObject);
		}
		displayDate.x = json_object_get_int(keyObject);

		if (!json_object_object_get_ex(rootObject, "y", &keyObject))
		{
			keyObject = json_object_new_int(30);
			json_object_object_add(rootObject, "y", keyObject);
		}
		displayDate.y = json_object_get_int(keyObject);

		if (!json_object_object_get_ex(rootObject, "show", &keyObject))
		{
			keyObject = json_object_new_boolean(1);
			json_object_object_add(rootObject, "show", keyObject);
		}
		displayDate.show = json_object_get_boolean(keyObject);

		if (!json_object_object_get_ex(rootObject, "color", &keyObject))
		{
			keyObject = json_object_new_string("FF00FF");
			json_object_object_add(rootObject, "color", keyObject);
		}
		convRGBstr((char *)json_object_get_string(keyObject), &displayDate.red, &displayDate.green, &displayDate.blue);
	}
	// printf("JSON(date):%s\n", json_object_get_string(rootObject));

	if (json_object_object_get_ex(ConfigJSON, "text", &rootObject))
	{
		struct json_object *keyObject;

		if (!json_object_object_get_ex(rootObject, "format", &keyObject))
		{
			keyObject = json_object_new_string("");
			json_object_object_add(rootObject, "format", keyObject);
		}
		displayText.format = json_object_get_string(keyObject);

		if (!json_object_object_get_ex(rootObject, "dir", &keyObject))
		{
			keyObject = json_object_new_string("/usr/local/share/fonts/rgbmatrix/");
			json_object_object_add(rootObject, "dir", keyObject);
		}
		displayText.dir = json_object_get_string(keyObject);

		if (!json_object_object_get_ex(rootObject, "font", &keyObject))
		{
			keyObject = json_object_new_string("6x9.bdf");
			json_object_object_add(rootObject, "font", keyObject);
		}
		displayText.fontname = json_object_get_string(keyObject);
		fetchFont(&displayText);

		if (!json_object_object_get_ex(rootObject, "x", &keyObject))
		{
			keyObject = json_object_new_int(0);
			json_object_object_add(rootObject, "x", keyObject);
		}
		displayText.x = json_object_get_int(keyObject);

		if (!json_object_object_get_ex(rootObject, "y", &keyObject))
		{
			keyObject = json_object_new_int(30);
			json_object_object_add(rootObject, "y", keyObject);
		}
		displayText.y = json_object_get_int(keyObject);

		if (!json_object_object_get_ex(rootObject, "show", &keyObject))
		{
			keyObject = json_object_new_boolean(0);
			json_object_object_add(rootObject, "show", keyObject);
		}
		displayText.show = json_object_get_boolean(keyObject);

		if (!json_object_object_get_ex(rootObject, "color", &keyObject))
		{
			keyObject = json_object_new_string("888888");
			json_object_object_add(rootObject, "color", keyObject);
		}
		convRGBstr((char *)json_object_get_string(keyObject), &displayText.red, &displayText.green, &displayText.blue);
	}
	// printf("JSON(date):%s\n", json_object_get_string(rootObject));


	Magick::InitializeMagick(*argv);

	rgb_matrix::RuntimeOptions runtime_opt;
	if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &panelOptions, &runtime_opt))
	{
		return usage(argv[0]);
	}

	// Prepare matrix
	canvas = CreateMatrixFromOptions(panelOptions, runtime_opt);
	if (canvas == NULL)
		return 1;

//	if (large_display)
//	{
//		// Mapping the coordinates of a 32x128 display mapped to a square of 64x64,
//		// or any other U-shape.
//		canvas->ApplyStaticTransformer(rgb_matrix::UArrangementTransformer( panelOptions.parallel));
//	}

	if (angle >= -360)
	{
		canvas->ApplyStaticTransformer(rgb_matrix::RotateTransformer(angle));
	}

	offscreen_canvas = canvas->CreateFrameCanvas();
	printf("Panel size: %dx%d. Hardware gpio mapping: %s\n", canvas->width(), canvas->height(), panelOptions.hardware_mapping);

	LoadImage(displayImage.image);

	/*
	struct dirent *pDirent;
	DIR *pDir;
	pDir = opendir(displayImage.dir);
	if (pDir == NULL)
		printf("Can't open '%s'!\n", displayImage.dir);
	else
	{
		chdir(displayImage.dir);
		while ((pDirent = readdir(pDir)) != NULL)
		{
			if(pDirent->d_type==DT_DIR)
				continue;

			fprintf(stderr, "Loading file: %s\n", pDirent->d_name);
			LoadImage(pDirent->d_name);

			// Set defaults.
			ImageParams img_param;
			filename_params[pDirent->d_name] = img_param;
		}
	}
	closedir(pDir);
	*/

	return 0;
}


bool fetchFont(_Text *Text)
{
	char Filename[128] = {0};
	strcpy(Filename, Text->dir);
	strcat(Filename, "/");
	strcat(Filename, Text->fontname);

	printf("Loading font:'%s'..", Filename);
	if (!Text->ptrFont.LoadFont(Filename))
	{
		printf("Error\n");
		return(1);
	}
	printf("Done\n");
	return(0);
}


void LoadImage(const char *filename)
{
	// These parameters are needed once we do scrolling.
	const bool fill_width = false;
	const bool fill_height = false;

	chdir(displayImage.dir);
	std::string err_msg;
	std::vector<Magick::Image> image_sequence;
	printf("Loading File:%s ...", filename);
	if (LoadImageAndScale(filename, canvas->width(), canvas->height(), fill_width, fill_height, &image_sequence, &err_msg))
	{
		content_stream = new rgb_matrix::MemStreamIO();
		bgImage.is_multi_frame = image_sequence.size() > 1;
		rgb_matrix::StreamWriter out(content_stream);
		for (size_t i = 0; i < image_sequence.size(); ++i)
		{
			const Magick::Image &img = image_sequence[i];
			int64_t delay_time_us;
			if (bgImage.is_multi_frame)
				delay_time_us = img.animationDelay() * 10000; // unit in 1/100s
			else
				delay_time_us = bgImage.wait_ms * 1000;  // single image.
			if (delay_time_us <= 0) delay_time_us = 100 * 1000;  // 1/10sec
				StoreInStream(img, delay_time_us, do_center, offscreen_canvas, global_stream_writer ? global_stream_writer : &out);
			printf(".");
		}
		printf(".Done\n");
	}
	else
		printf(".Not done.\n");
}


// ################################################################################
static tmillis_t GetTimeInMillis()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}


static void SleepMillis(tmillis_t milli_seconds)
{
	if (milli_seconds <= 0) return;
	struct timespec ts;
	ts.tv_sec = milli_seconds / 1000;
	ts.tv_nsec = (milli_seconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
}


static void StoreInStream(const Magick::Image &img, int delay_time_us, bool do_center, rgb_matrix::FrameCanvas *scratch, rgb_matrix::StreamWriter *output)
{
	scratch->Clear();
	const int x_offset = do_center ? (scratch->width() - img.columns()) / 2 : 0;
	const int y_offset = do_center ? (scratch->height() - img.rows()) / 2 : 0;
	for (size_t y = 0; y < img.rows(); ++y)
	{
		for (size_t x = 0; x < img.columns(); ++x)
		{
			const Magick::Color &c = img.pixelColor(x, y);
			if (c.alphaQuantum() < 256)
			{
				scratch->SetPixel(x + x_offset, y + y_offset, ScaleQuantumToChar(c.redQuantum()), ScaleQuantumToChar(c.greenQuantum()), ScaleQuantumToChar(c.blueQuantum()));
			}
		}
	}
	output->Stream(*scratch, delay_time_us);
}


// Load still image or animation.
// Scale, so that it fits in "width" and "height" and store in "result".
static bool LoadImageAndScale(const char *filename, int target_width, int target_height, bool fill_width, bool fill_height, std::vector<Magick::Image> *result, std::string *err_msg)
{
	std::vector<Magick::Image> frames;
	try
	{
		readImages(&frames, filename);
	}
	catch (std::exception& e)
	{
		if (e.what()) *err_msg = e.what();
			return false;
	}
	if (frames.size() == 0)
	{
		fprintf(stderr, "No image found.");
		return false;
	}

	// Put together the animation from single frames. GIFs can have nasty
	// disposal modes, but they are handled nicely by coalesceImages()
	if (frames.size() > 1)
		Magick::coalesceImages(result, frames.begin(), frames.end());
	else
		result->push_back(frames[0]);   // just a single still image.

	const int img_width = (*result)[0].columns();
	const int img_height = (*result)[0].rows();
	const float width_fraction = (float)target_width / img_width;
	const float height_fraction = (float)target_height / img_height;
	if (fill_width && fill_height)
	{
		// Scrolling diagonally. Fill as much as we can get in available space.
		// Largest scale fraction determines that.
		const float larger_fraction = (width_fraction > height_fraction) ? width_fraction : height_fraction;
		target_width = (int) roundf(larger_fraction * img_width);
		target_height = (int) roundf(larger_fraction * img_height);
	}
	else if (fill_height)
	{
		// Horizontal scrolling: Make things fit in vertical space.
		// While the height constraint stays the same, we can expand to full
		// width as we scroll along that axis.
		target_width = (int) roundf(height_fraction * img_width);
	}
	else if (fill_width)
	{
		// dito, vertical. Make things fit in horizontal space.
		target_height = (int) roundf(width_fraction * img_height);
	}

	for (size_t i = 0; i < result->size(); ++i)
	{
		(*result)[i].scale(Magick::Geometry(target_width, target_height));
	}

	return true;
}


void DisplayAnimation(RGBMatrix *matrix, FrameCanvas *offscreen_canvas, int vsync_multiple)
{
	time_t rawtime;
	struct tm *now;
	char theTime[80];

	const tmillis_t duration_ms = (bgImage.is_multi_frame ? bgImage.anim_duration_ms : bgImage.wait_ms);
	rgb_matrix::StreamReader reader(content_stream);
	int loops = bgImage.loops;
	const tmillis_t end_time_ms = GetTimeInMillis() + duration_ms;
	const tmillis_t override_anim_delay = bgImage.anim_delay_ms;

	for (int k = 0; (loops < 0 || k < loops) && !Interrupt && GetTimeInMillis() < end_time_ms; ++k)
	{
		uint32_t delay_us = 0;
		while (!Interrupt && GetTimeInMillis() <= end_time_ms && reader.GetNext(offscreen_canvas, &delay_us))
		{
			const tmillis_t anim_delay_ms = override_anim_delay >= 0 ? override_anim_delay : delay_us / 1000;
			const tmillis_t start_wait_ms = GetTimeInMillis();

			// Display the time.
			if (displayTime.show)
			{
				time(&rawtime);
				now = localtime(&rawtime);
				strftime(theTime, 80, displayTime.format, now);
				//printf("Time: %s\n", theTime);

				Color color(displayTime.red, displayTime.green, displayTime.blue);
				rgb_matrix::DrawText(offscreen_canvas, displayTime.ptrFont, displayTime.x, displayTime.y + displayTime.ptrFont.baseline(), color, NULL, theTime, 0);
			}

			// Display the date.
			if (displayDate.show)
			{
				time(&rawtime);
				now = localtime(&rawtime);
				strftime(theTime, 80, displayDate.format, now);
				//printf("Date: %s\n", theTime);

				Color color(displayDate.red, displayDate.green, displayDate.blue);
				rgb_matrix::DrawText(offscreen_canvas, displayDate.ptrFont, displayDate.x, displayDate.y + displayDate.ptrFont.baseline(), color, NULL, theTime, 0);
			}

			// Display arbitrary text.
			if (displayText.show)
			{
				Color color(displayText.red, displayText.green, displayText.blue);
				if (displayText.format)
					rgb_matrix::DrawText(offscreen_canvas, displayText.ptrFont, displayText.x, displayText.y + displayText.ptrFont.baseline(), color, NULL, displayText.format, 0);
			}

			offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas, vsync_multiple);
			const tmillis_t time_already_spent = GetTimeInMillis() - start_wait_ms;
			SleepMillis(anim_delay_ms - time_already_spent);
		}
		reader.Rewind();
	}
}


// ################################################################################
int initMQTT(void)
{
	// uint8_t reconnect = true;
	char clientid[24];
	int rc = 0;

	mosquitto_lib_init();

	memset(clientid, 0, 24);
	snprintf(clientid, 23, "mysql_log_%d", getpid());
	mosq = mosquitto_new(clientid, true, 0);
	if(mosq)
	{
		mosquitto_connect_callback_set(mosq, connect_callback);
		mosquitto_message_callback_set(mosq, message_callback);

		rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);

		mosquitto_subscribe(mosq, NULL, "/display/#", 0);
	}

	return(rc);
}


int runMQTT(void)
{
	int rc = 0;

	// MQTT client.
	if(mosq)
	{
		//rc = mosquitto_loop(mosq, -1, 1);
		rc = mosquitto_loop(mosq, 0, 1);
		if(Interrupt && rc)
		{
			printf("connection error!\n");
			sleep(10);
			mosquitto_reconnect(mosq);
		}
	}

	return(rc);
}


void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	printf("connect callback, rc=%d\n", result);
}


void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	bool match = false;
	int payloadInt = 0x00;
	// int payloadInt = std::stoi((char *)message->payload, nullptr, 0);
	if (message->payloadlen > 0)
		payloadInt = strtol((char *)message->payload, nullptr, 0);
	printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);


	// Background image topics.
	mosquitto_topic_matches_sub("/display/panel/#", message->topic, &match);
	if (match)
	{
		mosquitto_topic_matches_sub("/display/panel/brightness", message->topic, &match);
		if (match)
		{
			setPanelConfig((char *)"brightness", (char *)message->payload, false);
			printf("Set brightness: '%s'\n", (char *)message->payload);
		}
	}

	mosquitto_topic_matches_sub("/display/background/load", message->topic, &match);
	if (match)
	{
		displayImage.image = (char *)message->payload;
		LoadImage(displayImage.image);
	}


	// Time topics.
	mosquitto_topic_matches_sub("/display/time/format", message->topic, &match);
	if (match)
		displayTime.format = (char *)message->payload;

	mosquitto_topic_matches_sub("/display/time/show", message->topic, &match);
	if (match)
		displayTime.show = payloadInt;

	mosquitto_topic_matches_sub("/display/time/x", message->topic, &match);
	if (match && (payloadInt >= 0) && (payloadInt < canvas->width()))
		displayTime.x = payloadInt;

	mosquitto_topic_matches_sub("/display/time/y", message->topic, &match);
	if (match && (payloadInt >= 0) && (payloadInt < canvas->width()))
		displayTime.y = payloadInt;

	mosquitto_topic_matches_sub("/display/time/dir", message->topic, &match);
	if (match)
		displayTime.dir = (char *)message->payload;

	mosquitto_topic_matches_sub("/display/time/font", message->topic, &match);
	if (match)
	{
		displayTime.fontname = (char *)message->payload;
		fetchFont(&displayTime);
	}

	mosquitto_topic_matches_sub("/display/time/color", message->topic, &match);
	if (match)
		convRGBstr((char *)message->payload, &displayTime.red, &displayTime.green, &displayTime.blue);


	// Date topics.
	mosquitto_topic_matches_sub("/display/date/format", message->topic, &match);
	if (match)
		displayDate.format = (char *)message->payload;

	mosquitto_topic_matches_sub("/display/date/show", message->topic, &match);
	if (match)
		displayDate.show = payloadInt;

	mosquitto_topic_matches_sub("/display/date/x", message->topic, &match);
	if (match && (payloadInt >= 0) && (payloadInt < canvas->width()))
		displayDate.x = payloadInt;

	mosquitto_topic_matches_sub("/display/date/y", message->topic, &match);
	if (match && (payloadInt >= 0) && (payloadInt < canvas->width()))
		displayDate.y = payloadInt;

	mosquitto_topic_matches_sub("/display/date/dir", message->topic, &match);
	if (match)
		displayDate.dir = (char *)message->payload;

	mosquitto_topic_matches_sub("/display/date/font", message->topic, &match);
	if (match)
	{
		displayDate.fontname = (char *)message->payload;
		fetchFont(&displayDate);
	}

	mosquitto_topic_matches_sub("/display/date/color", message->topic, &match);
	if (match)
		convRGBstr((char *)message->payload, &displayDate.red, &displayDate.green, &displayDate.blue);


	// Text topics.
	mosquitto_topic_matches_sub("/display/text/format", message->topic, &match);
	if (match)
	{
		if (message->payloadlen == 0x00)
			displayText.show = false;
		else
		{
			displayText.show = true;
			strcpy((char *)displayText.format, (char *)message->payload);
		}
	}

	mosquitto_topic_matches_sub("/display/text/show", message->topic, &match);
	if (match)
		displayText.show = payloadInt;

	mosquitto_topic_matches_sub("/display/text/x", message->topic, &match);
	if (match && (payloadInt >= 0) && (payloadInt < canvas->width()))
	{
		printf("Int:%d\n", payloadInt);
		displayText.x = payloadInt;
	}

	mosquitto_topic_matches_sub("/display/text/y", message->topic, &match);
	if (match && (payloadInt >= 0) && (payloadInt < canvas->width()))
		displayText.y = payloadInt;

	mosquitto_topic_matches_sub("/display/text/dir", message->topic, &match);
	if (match)
		displayText.dir = (char *)message->payload;

	mosquitto_topic_matches_sub("/display/text/font", message->topic, &match);
	if (match)
	{
		displayText.fontname = (char *)message->payload;
		fetchFont(&displayText);
	}

	mosquitto_topic_matches_sub("/display/text/color", message->topic, &match);
	if (match)
		convRGBstr((char *)message->payload, &displayText.red, &displayText.green, &displayText.blue);
}


// ################################################################################
static int usage(const char *progname)
{
	fprintf(stderr, "usage: %s [options] <image> [option] [<image> ...]\n", progname);

	fprintf(stderr, "Options:\n"
		"\t-O<streamfile>            : Output to stream-file instead of matrix (Don't need to be root).\n"
		"\t-C                        : Center images.\n"

		"\nThese options affect images following them on the command line:\n"
		"\t-w<seconds>               : Regular image: "
		"Wait time in seconds before next image is shown (default: 1.5).\n"
		"\t-t<seconds>               : "
		"For animations: stop after this time.\n"
		"\t-l<loop-count>            : "
		"For animations: number of loops through a full cycle.\n"
		"\t-D<animation-delay-ms>    : "
		"For animations: override the delay between frames given in the\n"
		"\t                            gif/stream animation with this value. Use -1 to use default value.\n"

		"\nOptions affecting display of multiple images:\n"
		"\t-f                        : "
		"Forever cycle through the list of files on the command line.\n"
		"\t-s                        : If multiple images are given: shuffle.\n"
		"\nDisplay Options:\n"
		"\t-V<vsync-multiple>        : Expert: Only do frame vsync-swaps on multiples of refresh (default: 1)\n"
		"\t-L                        : Large display, in which each chain is 'folded down'\n"
		"\t                            in the middle in an U-arrangement to get more vertical space.\n"
		"\t-R<angle>                 : Rotate output; steps of 90 degrees\n"
		);

	fprintf(stderr, "\nGeneral LED matrix options:\n");
	rgb_matrix::PrintMatrixFlags(stderr);

	fprintf(stderr,
		"\nSwitch time between files: "
		"-w for static images; -t/-l for animations\n"
		"Animated gifs: If both -l and -t are given, "
		"whatever finishes first determines duration.\n");

	fprintf(stderr, "\nThe -w, -t and -l options apply to the following images "
		"until a new instance of one of these options is seen.\n"
		"So you can choose different durations for different images.\n");

	return(1);
}

