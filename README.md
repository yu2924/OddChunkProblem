# OddChunkProblem

## A problem about reading odd-sized metadata in wav files with JUCE framework 7.0.8

This code reproduces a certain problem when reading metadata in wav files using juce::AudioFileReader.

I noticed that if the subchunks in the INFO chunk are odd-sized, the WavAudioFormatReader loses track of the next subchunk.

The RIFF specification requires the following
* If the chunk size is odd, insert a padding byte to align with WORD boundary
* the cksize field of the chunk header does not include padding

Therefore, it is legal for the cksize field of a chunk header to have an odd value. As actual examples, following applications are found to output such odd-sized metadata chunks.
* Adobe Audition CS6
* Soundforge Pro 13

In my opinion, the ListInfoChunk::addToMetadata() [juce_WavAudioFormat.cpp:774] loop is missing a step to skip padding if infoLength is odd.

Let's make it sure.  
This code generates four odd-sized metadata and reads it with AudioFormatReader, and output as follows:
```
-- metadata begin --
"ICMT"="ICMT: odd"
"MetaDataSource"="WAV"
-- metadata end --
```
The result is that only the first metadata can be read.  
Now add changes to the ListInfoChunk::addToMetadata() method:
```
if (infoLength > 0)
{
    ...

    // add this line to the end of the block
    if(infoLength & 0x01) input.skipNextBytes(1);
}

```
Then, The output then changes as follows:
```
-- metadata begin --
"ICMT"="ICMT: odd"
"CMNT"="CMNT: odd"
"COMM"="COMM: odd"
"MetaDataSource"="WAV"
"IKEY"="IKEY: odd"
-- metadata end --
```
Now all four metadata can be read.

## Written by

[yu2924](https://twitter.com/yu2924)

## License

CC0 1.0 Universal
