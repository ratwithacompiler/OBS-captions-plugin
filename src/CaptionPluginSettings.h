//
// Created by Rat on 06.10.19.
//

#ifndef OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONPLUGINSETTINGS_H
#define OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONPLUGINSETTINGS_H


#include "SourceCaptioner.h"
#include <sstream>

struct CaptionPluginSettings {
    bool enabled;
    SourceCaptionerSettings source_cap_settings;

    CaptionPluginSettings(bool enabled, const SourceCaptionerSettings &source_cap_settings) :
            enabled(enabled),
            source_cap_settings(source_cap_settings) {}

    bool operator==(const CaptionPluginSettings &rhs) const {
        return enabled == rhs.enabled &&
               source_cap_settings == rhs.source_cap_settings;
    }

    bool operator!=(const CaptionPluginSettings &rhs) const {
        return !(rhs == *this);
    }

    string describe(const string &sep = "\n", const string &line_prefix = "") const {
        std::ostringstream os;
        os << line_prefix << "CaptionPluginSettings";
        os << sep << line_prefix << "enabled: " << enabled;
        os << sep << source_cap_settings.describe(sep, line_prefix);
        return os.str();
    }

    void print(const char *line_prefix = "") const {
        printf("%s\n", describe("\n", line_prefix).c_str());
    }
};


#endif //OBS_GOOGLE_CAPTION_PLUGIN_CAPTIONPLUGINSETTINGS_H
