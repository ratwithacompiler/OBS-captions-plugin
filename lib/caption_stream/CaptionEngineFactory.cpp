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

#include <exception>
#include <utility>

#include "ContinuousCaptions.h"
#include "CaptionEngineFactory.h"
#include "SherpaTOneCaptionEngine.h"

CaptionEngineCreationResult CaptionEngineFactory::create(
        CaptionEngineType type,
        const ContinuousCaptionStreamSettings &settings,
        const LocalCaptionEngineSettings &local_settings) {
    switch (type) {
        case CaptionEngineType::LocalSherpaTOne:
            try {
                return {
                        std::make_unique<SherpaTOneCaptionEngine>(local_settings),
                        CaptionEngineCreationStatus::Success,
                        "",
                };
            } catch (const std::exception &error) {
                return {
                        nullptr,
                        CaptionEngineCreationStatus::ConstructionFailed,
                        error.what(),
                };
            } catch (...) {
                return {
                        nullptr,
                        CaptionEngineCreationStatus::ConstructionFailed,
                        "The local caption engine constructor raised an unknown error.",
                };
            }
        case CaptionEngineType::GoogleHttpLegacy:
            try {
                return {
                        std::make_unique<ContinuousCaptions>(settings),
                        CaptionEngineCreationStatus::Success,
                        "",
                };
            } catch (const std::exception &error) {
                return {
                        nullptr,
                        CaptionEngineCreationStatus::ConstructionFailed,
                        error.what(),
                };
            } catch (...) {
                return {
                        nullptr,
                        CaptionEngineCreationStatus::ConstructionFailed,
                        "The caption engine constructor raised an unknown error.",
                };
            }
    }

    return {
            nullptr,
            CaptionEngineCreationStatus::UnsupportedEngine,
            "The requested caption engine is not available in this build.",
    };
}
