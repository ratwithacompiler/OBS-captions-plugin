Linux support is not well tested yet and requires OBS built with caption support (`-DBUILD_CAPTIONS=ON`) which most distro packages currently don't have enabled by default.

If it's not compiled with that the plugin will fail to load with an error like `error: os_dlopen(/some/path/libobs_google_caption_plugin.so): undefined symbol: obs_output_output_caption_text2` in the logs.

Install: 
* Put `libobs_google_caption_plugin.so` into your OBS plugins folder
  * Plugin folder location can vary a lot depending on distribution and/or installation source.
  * Usually also works in `$HOME/.config/obs-studio/plugins`
    * In that folder it needs to be in the required obs plugin folder structure:
    * `$HOME/.config/obs-studio/plugins/libobs_google_caption_plugin/bin/64bit/libobs_google_caption_plugin.so`



