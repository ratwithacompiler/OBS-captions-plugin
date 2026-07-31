/******************************************************************************
Copyright (C) 2026 AI Caption Plugin contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*******************************************************************************/

#ifndef AI_CAPTION_PLUGIN_CAPTION_ENGINE_FACTORY_H
#define AI_CAPTION_PLUGIN_CAPTION_ENGINE_FACTORY_H

#include <memory>
#include <string>

#include "CaptionEngine.h"
#include "LocalCaptionEngineSettings.h"

struct ContinuousCaptionStreamSettings;

enum class CaptionEngineType {
    LocalSherpaTOne,
    GoogleHttpLegacy,
};

enum class CaptionEngineCreationStatus {
    Success,
    UnsupportedEngine,
    ConstructionFailed,
};

struct CaptionEngineCreationResult {
    std::unique_ptr<CaptionEngine> engine;
    CaptionEngineCreationStatus status;
    std::string message;

    bool succeeded() const {
        return status == CaptionEngineCreationStatus::Success && engine != nullptr;
    }
};

class CaptionEngineFactory {
public:
    static CaptionEngineCreationResult create(
            CaptionEngineType type,
            const ContinuousCaptionStreamSettings &settings,
            const LocalCaptionEngineSettings &local_settings = {});
};

#endif // AI_CAPTION_PLUGIN_CAPTION_ENGINE_FACTORY_H
