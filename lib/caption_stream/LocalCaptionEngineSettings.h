/******************************************************************************
Copyright (C) 2026 AI Caption Plugin contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*******************************************************************************/

#ifndef AI_CAPTION_PLUGIN_LOCAL_CAPTION_ENGINE_SETTINGS_H
#define AI_CAPTION_PLUGIN_LOCAL_CAPTION_ENGINE_SETTINGS_H

#include <string>

struct LocalCaptionEngineSettings {
    std::string model_directory;
    unsigned int num_threads = 1;
    unsigned int max_pending_audio_ms = 1000;
};

#endif // AI_CAPTION_PLUGIN_LOCAL_CAPTION_ENGINE_SETTINGS_H
