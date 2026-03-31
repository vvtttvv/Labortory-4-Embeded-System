
#include "AppController.h"
#include "RoundRobinScheduler.h"

namespace {
AppController g_app;
RoundRobinScheduler g_scheduler(g_app);
}

void setup()
{
	g_app.setup();
	g_scheduler.start();
}

void loop(){ }