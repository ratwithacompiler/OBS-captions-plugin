## OBS Closed Captioning Plugin using Google Speech Recognition API

Captions a OBS audio source using Google's online Speech Recognition API. 
OBS captions use Twitch's built in player caption support and work on livestreams and in VODs on PC and iOS. 

Captions should be turned off by default for most users but some will have it on by default for various reasons.

* On PC the captions can be enabled and disabled using the CC button on the bottom right of the player.
* On iOS it's a system wide setting: `Settings -> General -> Accessibility -> Subtitles & Captioning -> Closed Captions + SHD` 
  * if it's already off users might have turn it on and off again (appears to be a bug on some iOS version unfortunately)


#### Notes:
* The plugin only captions the selected audio source when it's not muted and when it's used on the current active scene to ensure safety and avoid any captioning when the audio is muted for stream.
* The delay is usually less than half a second and should not be noticeable to viewers.
* Only tested on `Twitch.tv`, other streaming services with native caption support might work but not very likely.
* Video players with caption support like VLC can also show captions on downloaded VODs if enabled.
* The quality of Google's Speech Recognition heavily depends on the speaker and what is being said. 
The results are usually be pretty good in normal conversational settings like talking to chat but the recognition quality can go down noticeably when using ingame terms or other specialized vocabulary or during hectic speaking.  

#### Requires OBS 23.2.1 (released June 15th 2019) or newer!

### Installation:

* Close OBS if running
* Download latest release from: https://github.com/ratwithacompiler/OBS-captions-plugin/releases
* Extract the release archive, it contains two folders named 32bit and 64bit
* Copy-Paste those two folders into the `obs-plugins` directory in your OBS install directory
  * The default OBS install path is usually `C:\Program Files\obs-studio\`
  * The `obs-plugins` directory will already contain a `32bit` or `64bit` directory, pasting these two folders will just merge them with the existing ones to install the plugin
  * If you already have an older version of the plugin installed it will ask you to overwrite the old files, select yes 
* Start OBS, there should now be a `Cloud Closed Captions` options on the bottom of `Tools` menu
  * NOT `Captions (Experimental)` that's the built in captions using Windows speech recognition
* Click on `Settings` in the new `Captions Preview` window and select your audio source for captioning under `Caption Source`
* Recommended settings: 3 lines (4 lines can have flickering issues on Twitch currently!), no forced linebreaks


##### [Example VOD here](https://www.twitch.tv/videos/441407980?t=20s)

##### Screenshot:
![Example Image](https://i.imgur.com/BjeMg0W.png)

