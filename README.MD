# cassave
## Overview
cassave is a utility to turn digital files into audio that can be recorded onto physical media.
It also reverses to process to produce the exact file that was put in.

## Features
- Configurable bitrate, frequency, and samplerate
- Best-case expansion of 1KiB data to 3 seconds audio
- Automatic error checking using CRC32

## Installation
Clone the repo and run

## Usage
The simplest use is `cassave -i inputFile` which outputs to `output.wav`

To reverse this, simply use `cassave -i output.wav`

If encoding with parameters as outlined below, you MUST use them to decode as well.
This is outlined in Limitations.

### Configuration
Bitrate is changed using the `-b` flag.

It accepts values from 1 (very slow, highly stable) to 5 (super fast, not stable for analogue).

The default is 2.


The frequencies used can be changed using `-f`

It accepts values from 1 (very low frequency) to 5 (very high, requires high sample rate).

The default is 2.


Sample rate can be changed using `-s`

It accepts values from 1 (very low) to 11 (unreasonably high).

The default is 5 (CD quality).


The full list of options and their respective values can be found using the `--help/-h` flag.

## Performance / Size
cassave is written in C for speed and minimal overhead.
On a typical laptop (with a SATA SSD) from 2015, it encodes 27KiB in approximately 1.4 seconds. Decoding takes
approximately 0.9 seconds.
On a mid-high range desktop (with NVME SSD) from 2020, both encoding and decoding take approximately 0.4 seconds.

File sizes:
- CLI Binary - 23K stripped
- Full CLI release - 20K compressed (7z)
- GUI Binary - 133K stripped
- Full GUI release - 53K compressed (7z)

## Dependencies
cassave uses libm (for maths) and libsndfile (for parsing the audio).

## Build
Just run `make` and it'll create the CLI version and a flaky X11 GUI version.

Run `make CLI` to just make the CLI version

## Limitations
There are plenty.

One of the most important is that it can't detect the encoding configuration (bitrate, frequency) so it segfaults if you don't specify on decoding.
It also segfaults if the input file creates a file too large to fit in RAM. This is because it doesn't stream so everything is in RAM.

## License
Licensed under GPLv3.
Read COPYING for details.

