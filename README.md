## Closed Captioning OBS Plugin

Provides closed captioning via Google Cloud Speech Recognition API as a standalone OBS plugin, no other tools required. 
It's fully optional to viewers and uses Twitch's built in caption support which works on livestreams and in VODs on PC, Android and iOS, no Twitch extension required.  

#### Features:
  * Completely optional for viewers
  * Captions only when the microphone source is unmuted and active to ensure safety
  * Works live and in VODs, no Twitch extension required
  * Requires no extra tools or website open
  * Supports many common languages with western character sets
  * Supports OBS delay
  * Open Caption support via OBS Text Sources for sites that don't support closed captions
  * Supports saving captions to SRT Subtitle files (.srt)

##### Notes:
* The caption delay is usually less than half a second and should not be noticeable to viewers.
* The plugin only captions the selected audio source when it's not muted and when it's used on the current active scene to ensure safety and avoid any captioning when the mic is muted for the stream.
* Only tested on `Twitch.tv`, other streaming services with native caption support might work but not very likely.
* You can enable and disable the caption preview dock in OBS under `View -> Docks -> Captions` 
* Some video players like MPV can show embedded captions on downloaded VODs but very few support that.
* The quality of Google's Speech Recognition heavily depends on the speaker and what is being said. The results are usually be pretty good in normal conversational settings like talking to chat but the recognition quality can go down noticeably when using ingame terms or other specialized vocabulary or during hectic speaking. 
* SRT transcript files are generally recommended for saving captions for local recordings. They are support by many players, websites and tools and are easy to edit.

![Example Image](https://i.imgur.com/UcPk8gz.png)
##### [Example VOD here](https://www.twitch.tv/videos/441407980?t=20s)

##### Known Issues/Downsides:

Does NOT work with Streamlabs OBS (SLOBS).

Does NOT support any languages with foreign character sets like Japanese or Russian, that isn't possible the way Twitch and OBS captions work.

Does NOT appear to work with the AMD Hardware encoder in OBS on Windows (other hardware encoders like NVEnc and even AMD on MacOS seem fine)

This uses the built in captions support of Twitch's video player so viewers only get the limited amount of positioning options that the player provides. Viewers can choose between top/bottom and left/center/right for the captions box but can't freely move or resize it or put it into a corner. It's also not possible for streamers to pick a good custom default position for it, the default will be center bottom for all viewers that have captions enabled.
Viewers can use the [FrankerFaceZ Browser extension](https://chrome.google.com/webstore/detail/frankerfacez/fadndhdgpmmaapbmfcknlfgcflmmmieb) which provides the ability to set fully custom caption box positions and more settings.

Embedded Captions in local recordings currently aren't very useful. They only work with certain file formats (ts, mp4, mov) and only very few video players can correctly play them.
MPV can play them normally with .ts files, almost no common video players play them in mp4 and mov files. Saving transcripts to separate .srt subtitle files is generally much more useful.

Captions should be off by default for most viewers but Twitch does sometimes have them enabled for some viewers for unknown reasons so occasionally some will be confused on how to turn them off and might need it explained.

* **On PC** viewers can turn captions on and off using the CC button on the bottom right of the player.

    ![PC CC Setting](https://i.imgur.com/jBTzQT8.png)
    
    
* **On iOS** it's a system wide setting: `Settings -> General -> Accessibility -> Subtitles & Captioning -> Closed Captions + SHD` 
  * If it's already off but viewers still see captions they have to turn it on and off again (appears to be a bug on some iOS versions)
  
* **On Android** it's `Closed Captions` under the player settings options right beneath the quality selection. The option will only show up once the streamer has talked.

### Installation (Windows):
#### Requires OBS 23.2.1 (released June 15th 2019) or newer!

* Close OBS if running
* Download latest Closed_Captions_Plugin.zip version for Windows [from the releases section](https://github.com/ratwithacompiler/OBS-captions-plugin/releases)
* Extract the zip
  * it contains a folder named `obs-plugins`
* Go to your OBS installation folder
  * The default OBS install path is usually `C:\Program Files\obs-studio\` or `C:\Program Files (x86)\obs-studio\`
* Copy-Paste the `obs-plugins` folder into the main OBS folder
  * The main OBS folder should already contain `obs-plugins` `bin` and `data` folders
* Windows will ask to confirm the copy and replace, click yes a few times
  * This will just add the plugin files to the existing `obs-plugins` folder
* That's it. Start OBS.
* There should now be a `Cloud Closed Captions` option on the bottom of `Tools` menu
* Click on `Settings` in the new `Captions Preview` window and select your audio source for captioning under `Caption Source`
  * Select the OBS source that is only your microphone for best captioning results
  * If you don't have a OBS source that is only your microphone but instead use a more complicated audio setup see below for more info
* Recommended settings: 3 lines (4 lines can have flickering issues on Twitch currently!), no forced linebreaks

![Installation Windows](https://i.imgur.com/8EknThL.png)

#### Plugin:
![Example of Plugin in OBS](https://i.imgur.com/Xpp6HCe.png)

### Installation (Mac OS):
#### Requires OBS 24 or newer!

* Close OBS if running
* Download latest Closed_Captions_Plugin.zip version for MacOS [from the releases section](https://github.com/ratwithacompiler/OBS-captions-plugin/releases)
* Extract the zip
  * it contains a folder named `cloud_captions_plugin`
* Copy-Paste the `cloud_captions_plugin` folder into your OBS `plugins` folder
  * Open Finder and in the `Go` menu press `Go to Folder` (or press Cmd+Shift+G)  
  * Paste `~/Library/Application Support/obs-studio/` and hit enter to go to your `obs-studio` folder 
  * Your `obs-studio` folder may contain a folder called `plugins` already, if not then create one
  * Paste the `cloud_captions_plugin` folder into the `plugins` folder 
* That's it. Start OBS.
* There should now be a `Cloud Closed Captions` option on the bottom of `Tools` menu
* Click on `Settings` in the new `Captions Preview` window and select your audio source for captioning under `Caption Source`
  * Select the OBS source that is only your microphone for best captioning results
  * If you don't have a OBS source that is only your microphone but instead use a more complicated audio setup see below for more info
* Recommended settings: 3 lines (4 lines can have flickering issues on Twitch currently!), no forced linebreaks

![Installation Mac](https://i.imgur.com/nlF3TMr.png)

### Installation (Linux):

Linux support is not well tested yet and requires OBS built with caption support (`-DBUILD_CAPTIONS=ON`).

* Download latest Closed_Captions_Plugin.zip version for Linux [from the releases section](https://github.com/ratwithacompiler/OBS-captions-plugin/releases)
* Put `libobs_google_caption_plugin.so` into your OBS plugins folder
  * Plugin folder location can vary a lot depending on distribution and/or installation source.
  * Usually also works in `$HOME/.config/obs-studio/plugins`
    * In that folder it needs to be in the required obs plugin folder structure:
    * `$HOME/.config/obs-studio/plugins/libobs_google_caption_plugin/bin/64bit/libobs_google_caption_plugin.so`


#### Settings for more unusual audio setups that don't use a mic only source in OBS (like 2 PC + audio mixer setups):

If your OBS setup does not use a audio source that's only the microphone but instead gets a already mixed audio source (that has the microphone mixed with other sounds/game/voices already like from a mixer or GoXLR) you can still make it work as long as your streaming PC has access to only the microphone by itself as well. Steps:

* If you don't already have a OBS source that's only your mic then create one:
	* Create a new Audio Input Capture source in OBS somewhere using the device that's only the microphone, calling it `Microphone only` for example, and mute it (so stream doesn't hear the mic twice)
* In caption settings set `Caption Source` to the mic only OBS source that's muted
* Set `Caption When` to `Mute Source is heard on stream`
* Set `Mute Source` to your mixed audio OBS source that stream hears

![Mixed Source Setup Instructions](https://i.imgur.com/wuE89ZT.png)

This way it will use the `Microphone only` source to get clean mic audio for captioning but it will still only caption it when the other selected source is unmuted and active so it's still safe. When the mixed source for stream is muted in OBS the captions also get muted.
