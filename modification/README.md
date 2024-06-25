replace the source file "modules/juce_audio_formats/codecs/juce_WavAudioFormat.cpp" with this.

added the following lines at 799:
```
// added by yu2924
if(infoLength & 0x01) input.skipNextBytes(1);
```
