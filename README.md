## Closed Captioning OBS Plugin using Google Speech Recognition API

Provides closed captioning via Google Cloud Speech Recognition API as a standalone OBS plugin, no other tools required. 
It's fully optional to viewers and uses Twitch's built in caption support which works on livestreams and in VODs on PC and iOS, no Twitch extension needed.  

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
* The quality of Google's Speech Recognition heavily depends on the speaker and what is being said. 
The results are usually be pretty good in normal conversational settings like talking to chat but the recognition quality can go down noticeably when using ingame terms or other specialized vocabulary or during hectic speaking. 

##### Known Issues/Downsides:

Captions should be off by default for most viewers but Twitch does sometimes have them enabled for some viewers for unknown reasons so occasionally some will be confused on how to turn them off and might need it explained.

* **On PC** viewers can turn captions on and off using the CC button on the bottom right of the player.

    ![Example Image](https://i.imgur.com/jBTzQT8.png)
    
    
* **On iOS** it's a system wide setting: `Settings -> General -> Accessibility -> Subtitles & Captioning -> Closed Captions + SHD` 
  * If it's already off but viewers still see captions they have to turn it on and off again (appears to be a bug on some iOS versions)
  
Does NOT work with SLOBS (Streamlabs OBS).
    

### Installation:
#### Requires OBS 23.2.1 (released June 15th 2019) or newer!

* Close OBS if running
* Download latest Closed_Captions_Plugin.zip version [from the releases section](https://github.com/ratwithacompiler/OBS-captions-plugin/releases)
* Extract the zip
  * it contains two folders named `32bit` and `64bit` each containing a `obs_google_caption_plugin.dll` file
* Go to your OBS installation folder    
  * The default OBS install path is usually `C:\Program Files\obs-studio\` or `C:\Program Files (x86)\obs-studio\`
* For 32bit OBS installations: [1]
    * Copy-paste the `32bit\obs_google_caption_plugin.dll` file into the `obs-plugins\32bit\` folder in your OBS folder
* For 64bit OBS installations: [1]
    * Copy-paste the `64bit\obs_google_caption_plugin.dll` file into the `obs-plugins\64bit\` folder in your OBS folder
* That's it. Start OBS.
* There should now be a `Cloud Closed Captions` option on the bottom of `Tools` menu
  * NOT `Captions (Experimental)` that's the built in captions using Windows speech recognition
* Click on `Settings` in the new `Captions Preview` window and select your audio source for captioning under `Caption Source`
  * Select whatever OBS source is only your microphone for best captioning results
  * If you don't have a OBS source that is only your microphone but instead use a more complicated audio setup see below for more info
* Recommended settings: 3 lines (4 lines can have flickering issues on Twitch currently!), no forced linebreaks

[1] If you're not sure what OBS version you have: the `obs-plugins` folder should only contain either a `32bit` or `64bit` folder

##### [Example VOD here](https://www.twitch.tv/videos/441407980?t=20s)

##### Example Screenshot:
![Example Image](https://i.imgur.com/BjeMg0W.png)


#### Settings for more unusual audio setups that don't use a mic only source in OBS (like 2 PC + audio mixer setups):

If your OBS setup does not use a audio source that contains only the microphone but gets a already mixed audio source instead (that has the microphone mixed with other sounds/game/voices already) you can still make it work as long as your streaming PC has access to the microphone by itself as well. Steps:

* If you don't already have a OBS source that's only your mic:
	* Create a new Audio Input Capture source in OBS somewhere using the device that's only the microphone, calling it `Microphone only` for example, and mute it (so stream doesn't hear the mic twice)
* In caption settings set `Caption Source` to the mic only OBS source that's muted
* Set `Caption When` to `When Other Source is Streamed`
* Set `Other Source` to your mixed audio OBS source that stream hears

![Mixed Source Setup Instructions](https://i.imgur.com/CeWn5xw.png)

This way it will use the `Microphone only` source to get clean mic audio for captioning but it will still only caption it when the other selected source is unmuted and active so it's still safe for the streamer, when the mixed source for stream is muted in OBS the captions also get muted.

