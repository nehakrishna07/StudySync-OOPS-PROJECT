#pragma once
#include "../services/App.h"

// Handle one browser request, then return to the listening loop.
void serve(Socket client, App& app);
