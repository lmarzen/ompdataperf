#pragma once

#include "chunk.h"

#define TEALEAF_VERSION "2.000"

void initialise_model_info(Settings &settings);
void initialise_application(Chunk **chunks, Settings &settings, State * states);
bool diffuse(Chunk *chunk, Settings &settings);
void read_config(Settings &settings, State **states);

#ifdef DIFFUSE_OVERLOAD
bool diffuse_overload(Chunk *chunk, Settings &settings);
#endif
