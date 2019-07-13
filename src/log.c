#pragma once

#define do_log(level, format, ...) blog(level, "[dir_watch_media: '%s'] " format, obs_source_get_name(ss->source), ##__VA_ARGS__)
#define debug_log(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define info_log(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define warn_log(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
