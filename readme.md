**This is a copy of the [vamps](http://vamps.sourceforge.net/) source code at [v0.99.2](https://sourceforge.net/projects/vamps/files/Vamps/0.99.2/) from the [vamps sourceforge project](https://sourceforge.net/projects/vamps/)**

Vamps is a tool to help backing up DVDs: Vamps evaporates DVD compliant MPEG2 program streams by selectively copying audio and subpicture tracks and by re-quantizing the video stream. Shrink ratio may be based on video only or on the full PS.

# Usage

```
Usage: vamps [--evaporate|-e factor] [--ps-evaporate|-E factor]
             [--audio|-a a-stream,a-stream,...]
             [--subpictures|-s s-stream,s-stream,...] [--verbose|-v]
             [--inject|-i injections-file]
             [--preserve|-p] [--ps-size|-S input-bytes]

Vamps evaporates DVD compliant MPEG2 program streams by
selectively copying audio and subpicture streams and by re-quantizing
the embedded elementary video stream. The shrink factor may be either
specified for the video ES only (-e) or for the full PS (-E).
```

## Calculate the factor for DVD5 (4707MB)

```sh
echo $(du -BMB input.vob | cut -f 1 -d 'M')' / 4707' | bc -l

## macOS
brew install coreutils
echo $(gdu -BMB input.vob | cut -f 1 -d 'M')' / 4707' | bc -ls
```

## Example
```sh
vamps -E 1.14 -a 1 < input.vob > output.vob
```
