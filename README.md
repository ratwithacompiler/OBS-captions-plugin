## Closed Captioning OBS Plugin

Provides closed captioning via Google Cloud Speech Recognition API as a standalone OBS plugin, no other tools required. 
It's fully optional to viewers and uses Twitch's built in caption support which works on livestreams and in VODs on PC, Android and iOS, no Twitch extension required.  

#### Features:
  * Captions only when the microphone source is unmuted and active to ensure safety
  * Works live and in VODs, no Twitch extension required
  * Completely optional for viewers
  * Supports OBS delay
  * Requires no extra tools or website open
  * Supports many common languages with western character sets

##### Notes:
* The caption delay is usually less than half a second and should not be noticeable to viewers.
* The plugin only captions the selected audio source when it's not muted and when it's used on the current active scene to ensure safety and avoid any captioning when the mic is muted for the stream.
* Video players with caption support like VLC can also show captions on downloaded VODs if enabled.
* Only tested on `Twitch.tv`, other streaming services with native caption support might work but not very likely.
* You can enable and disable the caption preview dock in OBS under `View -> Docks -> Captions` 
* The quality of Google's Speech Recognition heavily depends on the speaker and what is being said. The results are usually be pretty good in normal conversational settings like talking to chat but the recognition quality can go down noticeably when using ingame terms or other specialized vocabulary or during hectic speaking. 

![Example Image](https://i.imgur.com/UcPk8gz.png)
##### [Example VOD here](https://www.twitch.tv/videos/441407980?t=20s)

##### Known Issues/Downsides:

Captions should be off by default for most viewers but Twitch does sometimes have them enabled for some viewers for unknown reasons so occasionally some will be confused on how to turn them off and might need it explained.

* **On PC** viewers can turn captions on and off using the CC button on the bottom right of the player.

    ![PC CC Setting](https://i.imgur.com/jBTzQT8.png)
    
    
* **On iOS** it's a system wide setting: `Settings -> General -> Accessibility -> Subtitles & Captioning -> Closed Captions + SHD` 
  * If it's already off but viewers still see captions they have to turn it on and off again (appears to be a bug on some iOS versions)
  
* **On Android** it's `Closed Captions` under the player settings options right beneath the quality selection. The option will only show up once the streamer has talked.
  
Does NOT work with Streamlabs OBS (SLOBS).

Does NOT appear to work with the AMD Hardware encoder in OBS on Windows (other hardware encoders like NvEnc and even AMD on MacOS seem fine)

This uses the built in captions support of Twitch's video player so viewers only get the limited amount of positioning options that the player provides. Viewers can choose between top/bottom and left/center/right for the captions box but can't freely move or resize it or put it into a corner. It's also not possible for streamers to pick a good custom default position for it, the default will be center bottom for all viewers that have captions enabled.
Viewers can use the [FrankerFaceZ Browser extension](https://chrome.google.com/webstore/detail/frankerfacez/fadndhdgpmmaapbmfcknlfgcflmmmieb) which provides the ability to set fully custom caption box positions and more settings.  

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
![Example of Plugin in OBS](https://i.imgur.com/ZfKnMoH.png)

### Installation (Mac OS):
#### Requires OBS 24 or newer!

* Close OBS if running
* Download latest Closed_Captions_Plugin.zip version for MacOS [from the releases section](https://github.com/ratwithacompiler/OBS-captions-plugin/releases)
* Extract the zip
  * it contains a file named `libobs_google_caption_plugin.so`
* Copy-Paste the `libobs_google_caption_plugin.so` file into your OBS `Plugins` folder
  * To find the `Plugins` folder go to your `Applications`
  * Right Click `OBS.app`
  * Select `Show Package Contents`
  * The `Plugins` folder is under `Contents` 
  * You may need to enter your password to copy the file into `Plugins`
* That's it. Start OBS.
* There should now be a `Cloud Closed Captions` option on the bottom of `Tools` menu
* Click on `Settings` in the new `Captions Preview` window and select your audio source for captioning under `Caption Source`
  * Select the OBS source that is only your microphone for best captioning results
  * If you don't have a OBS source that is only your microphone but instead use a more complicated audio setup see below for more info
* Recommended settings: 3 lines (4 lines can have flickering issues on Twitch currently!), no forced linebreaks

![Installation Mac](https://i.imgur.com/DVZISQI.png)



#### Settings for more unusual audio setups that don't use a mic only source in OBS (like 2 PC + audio mixer setups):

If your OBS setup does not use a audio source that's only the microphone but instead gets a already mixed audio source (that has the microphone mixed with other sounds/game/voices already like from a mixer or GoXLR) you can still make it work as long as your streaming PC has access to only the microphone by itself as well. Steps:

* If you don't already have a OBS source that's only your mic then create one:
	* Create a new Audio Input Capture source in OBS somewhere using the device that's only the microphone, calling it `Microphone only` for example, and mute it (so stream doesn't hear the mic twice)
* In caption settings set `Caption Source` to the mic only OBS source that's muted
* Set `Caption When` to `When Other Source is Streamed`
* Set `Other Source` to your mixed audio OBS source that stream hears

![Mixed Source Setup Instructions](https://i.imgur.com/CeWn5xw.png)

This way it will use the `Microphone only` source to get clean mic audio for captioning but it will still only caption it when the other selected source is unmuted and active so it's still safe. When the mixed source for stream is muted in OBS the captions also get muted.
