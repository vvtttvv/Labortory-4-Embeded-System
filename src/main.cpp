
#include "AppController.h"

namespace {
AppController g_app;
}

void setup()
{
	g_app.setup();
}

void loop()
{
	g_app.loop();
}