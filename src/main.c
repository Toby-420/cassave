// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Utility to encode binary data into waveform PCM using Frequency Shift Keying 
 *
 * Copyright (C) 2025 - 2026 Toby Valentine <tobyvalentine@members.fsf.org>
 * 
 * This program is free software: you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with 
 * this program. If not, see <https://www.gnu.org/licenses/>.
 * 
 *
 * This is a utility to encode digital files (including images, videos, text)
 * as waveform files. It could be used for storing files on audio tapes, or
 * storing insensitive data in an obscure way.
 *
 * What it can do:
 * - Encode/decode using different sample rates, bit rates, and tone frequencies.
 * - A best-case expansion of 1K to 3.25 seconds (290K of uncompressed WAV).
 * 
 * What is planned for inclusion:
 * - Compress using multiple ubiquitous compression algorithms (LZMA, BZIP2).
 * - Encryption with BZIP2.
 * - Pre and post-encoding specific (de)compression for certain types of file such
 *   as audio and video, rather than using general purpose algorithms such as LZMA.
 * */


#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h> /* for uintx_t */
#include <inttypes.h>

#include "../include/utils.h"

enum InfoFiles {
  HELP_FILE = 1,
  VERSION_FILE,
  WARRANTY_FILE,
};

#define _(str) gettext(str)

bool verbose = false;

struct 
option long_options[] = 
{
  { "help", no_argument, NULL, 'h' }, 
  // argument '--help' takes no additional arguments and has short code 'h'

  { "verbose", no_argument, NULL, 'V' },
  // argument '--verbose' takes no additional arguments and has short code 'V'
  
  { "input", required_argument, NULL, 'i' }, 
  // argument '--input' takes 1 required argument and has short code 'i'
  
  { "output", optional_argument, NULL, 'o' }, 
  // argument '--output' takes 1 optional argument and has short code 'o'
  
  { "version", no_argument, NULL, 'v' }, 
  // argument '--version' takes no additional arguments and has short code 'v'
  
  { "warranty", no_argument, NULL, 'w' }, 
  // argument '--warranty' takes no additional arguments and has short code 'w'

  // { "compression", required_argument, NULL, 'c' },
  // arg '--compression' needs and argument and has short code c

  { "bitrate", required_argument, NULL, 'b' },
  // arg bitrate needs an argument and has short code b

  { "frequency", required_argument, NULL, 'f' },
  // arg frequency needs an argument and has short code f

  { "sample-rate", required_argument, NULL, 's' },
  // arg sample-rate needs an argument and has short code s

  { 0, 0, 0, 0 },
  // if no argument is provided, accept that too
};

int 
main (int argc, char **argv) 
{
  int opt;
  char inputFile[261] = {"\0"};
  char outputFile[255] = "output.wav\0";
  uint8_t compressionFlag = 1; // Default LZMA LVL5
  // char password[128]; // for encryption
  uint8_t bitrate = 2;
  uint8_t bitFrequency = 2;
  uint8_t sampleRate = 4;

  // Take in user inputs using getopt_long from the GNU utils
  while ((opt = getopt_long(argc, argv, "i:o:vhrwb:f:s:V", long_options, NULL)) != -1)
  {
    switch (opt) 
    {
      // Display help information
      case 'h':
        displayInfo(HELP_FILE);
        return 0;
      // Display version number
      case 'v':
        displayInfo(VERSION_FILE);
        return 0;
      // Display warranty information
      case 'w':
        displayInfo(WARRANTY_FILE);
        return 0;
      // Store input file name into the correct place
      case 'i':
        snprintf(inputFile, 260, optarg);
        break;
      // Store output filename into the correct place
      case 'o':
        snprintf(outputFile, 255, optarg);
        break;
      // case 'c':
      //   compressionFlag = atoi(optarg);
      //   if (compressionFlag > 3 || compressionFlag < 0)
      //   {
      //     printf("Compression mode must be 0, 1, 2, or 3.\nSee --help for explanation\n");
      //     return -1;
      //   }
      //   if (compressionFlag == 3)
      //   {
      //     // Hilariously unsafe, but MVP and all that
      //     printf("You chose encryption. You'll need a password.\nMake it less than 128 chars: ");
      //     fgets(password, sizeof(password), stdin);
      //     // Remove newline if present
      //     password[strcspn(password, "\n")] = '\0';
      //   }
      //   if (verbose) printf("Compression Mode: %"PRIu8"\n", compressionFlag);
      //   break;
      case 'b':
        if (atoi(optarg) < 1 || atoi(optarg) > 5)
        {
          printf("Bitrate out of range\n");
          return -1;
        }

        bitrate = atoi(optarg);
        // if (strncmp(optarg, "XLOW", 4) || atoi(optarg) == 0)
        // {
        //   bitrate = 0;
        // } else if (strncmp(optarg, "LOW", 3) || atoi(optarg) == 1)
        // {
        //   bitrate = 1;
        // } else if (strncmp(optarg, "MED", 3) || atoi(optarg) == 2)
        // {
        //   bitrate = 2;
        // } else if (strncmp(optarg, "HIGH", 4) || atoi(optarg) == 3)
        // {
        //   bitrate = 3;
        // } else if (strncmp(optarg, "RHI", 3) || atoi(optarg) == 4)
        // {
        //   bitrate = 4;
        // } else if (strncmp(optarg, "XHI", 3) || atoi(optarg) == 5)
        // {
        //   bitrate = 5;
        // } else
        // {
        //   bitrate = 2;
        // }
        break;
      case 'f':
        if (atoi(optarg) < 1 || atoi(optarg) > 5)
        {
          printf("Frequency out of range\n");
          return -1;
        }
        bitFrequency = atoi(optarg);

        // if (strncmp(optarg, "XLOW", 4) || atoi(optarg) == 0)
        // {
        //   bitFrequency = 0;
        // } else if (strncmp(optarg, "LOW", 3) || atoi(optarg) == 1)
        // {
        //   bitFrequency = 1;
        // } else if (strncmp(optarg, "MED", 3) || atoi(optarg) == 2)
        // {
        //   bitFrequency = 2;
        // } else if (strncmp(optarg, "HIGH", 4) || atoi(optarg) == 3)
        // {
        //   bitFrequency = 3;
        // } else if (strncmp(optarg, "RHI", 3) || atoi(optarg) == 4)
        // {
        //   bitFrequency = 4;
        // } else if (strncmp(optarg, "XHI", 3) || atoi(optarg) == 5)
        // {
        //   bitFrequency = 5;
        // } else
        // {
        //   bitFrequency = 2;
        // }
        break;
      case 's':
        if (atoi(optarg) < 1 || atoi(optarg) > 11 )
        {
          printf("Sample rate out of range\n");
          return -1;
        }
        sampleRate = atoi(optarg);
        // if (strncmp(optarg, "TELE", 4) || atoi(optarg) == 0)
        // {
        //   sampleRate = 0;
        // } else if (strncmp(optarg, "SUB", 3) || atoi(optarg) == 1)
        // {
        //   sampleRate = 1;
        // } else if (strncmp(optarg, "VOIP", 4) || atoi(optarg) == 2)
        // {
        //   sampleRate = 2;
        // } else if (strncmp(optarg, "MPEG", 4) || atoi(optarg) == 3)
        // {
        //   sampleRate = 3;
        // } else if (strncmp(optarg, "CD", 2) || atoi(optarg) == 4)
        // {
        //   sampleRate = 4;
        // } else if (strncmp(optarg, "DVD", 3) || atoi(optarg) == 5)
        // {
        //   sampleRate = 5;
        // } else if (strncmp(optarg, "MASTER", 6) || atoi(optarg) == 6)
        // {
        //   sampleRate = 6;
        // } else if (strncmp(optarg, "BLURAY", 6) || atoi(optarg) == 7)
        // {
        //   sampleRate = 7;
        // } else if (strncmp(optarg, "HDCD", 4) || atoi(optarg) == 8)
        // {
        //   sampleRate = 8;
        // } else if (strncmp(optarg, "PRO", 3) || atoi(optarg) == 9)
        // {
        //   sampleRate = 9;
        // } else if (strncmp(optarg, "DXD", 3) || strncmp(optarg, "DxD", 3) || atoi(optarg) == 10)
        // {
        //   // In here for the name really
        //   sampleRate = 10;
        // } else if (strncmp(optarg, "MAX", 3) || atoi(optarg) == 11)
        // {
        //   sampleRate = 11;
        // } else
        // {
        //   sampleRate = 4;
        // }
        break;
      // Turn verbose on if necessary
      case 'V':
        verbose = true;
        break;
      // If not recognised, return -1 (error)
      case '?':
        return -1;
      // If nothing is captured, just display help information
      default:
        displayInfo (HELP_FILE);
        return 0;
    }
  }

  // The fun bit.
  // Here we detect if it's a WAV (decode the file)
  // or other (encode)

  // File to be input
  uint8_t *fileData = NULL;
  // Where the headers are stored
  uint32_t header[7] = {0};
  // 64 Bit file size (large maximum size
  size_t fileSize;
  // CRC32 is 32 bit (hence the name)
  uint32_t checksum;

  if (isWavFile(inputFile) == -1) // File doesn't exist---------------------------------------------------------------------
  {
    displayInfo (HELP_FILE);
    return -1;

  } else if (isWavFile(inputFile)) // File is WAV-----------------------------------------------------------------------------
  {
    // Decode
    printf("WAV file detected. Decoding.\n");

    int actualBitrate = 0;
    switch (bitrate)
    {
      case 0:
        // Extra Low
        actualBitrate = 1;
        break;
      case 1:
        // Low
        actualBitrate = 10;
        break;
      case 2:
        // Medium
        actualBitrate = 200;
        break;
      case 3:
        // High
        actualBitrate = 500;
        break;
      case 4:
        // Really high
        actualBitrate = 1000;
        break;
      case 5:
        // Extremely high
        actualBitrate = 2500;
        break;
      default:
        // Default value is 200 / medium
        actualBitrate = 200;
        break;
    }
    uint16_t actualFrequency = 0;
    switch (bitFrequency)
    {
      case 0:
        // Extra Low
        actualFrequency = 100;
        break;
      case 1:
        // Low
        actualFrequency = 400;
        break;
      case 2:
        // Medium
        actualFrequency = 1000;
        break;
      case 3:
        // High
        actualFrequency = 2000;
        break;
      case 4:
        // Really high
        actualFrequency = 8000;
        break;
      case 5:
        // Extremely high
        actualFrequency = 22000;
        break;
      default:
        // Default value is 1000 / medium
        actualFrequency = 1000;
        break;
    }

    decodeCassaveWav(inputFile, actualBitrate, actualFrequency);


  } else // File is not WAV------------------------------------------------------------------------------------------------------
  {
    // Encode
    printf("Non-WAV file detected. Encoding.\n");

    if (importFile(inputFile, &fileData, &fileSize) != 0)
    {
      fprintf(stderr, "Failed to import file\n");
      return -1;
    }

    // Only format basename (not path)
    char *base = strrchr(inputFile, '/');
    base = base ? base + 1 : inputFile;
    formatFilename(base);

    getFileSize(inputFile, &fileSize);

    createHeader(header, base, fileSize, compressionFlag);

    // Make cksum
    checksum = crc32_create(fileData, fileSize);
    if (verbose) printf("File checksum: %"PRIu32"\n", checksum);

    // Append header + binary data + crc
    // Find sizes of everything
    size_t headerBytes = 7 * sizeof(uint32_t);
    size_t crcBytes    = sizeof(uint32_t);
    size_t totalSize = headerBytes + fileSize + crcBytes;

    // Allocate the memory
    if (verbose) printf("Allocating output buffer... ");
    uint8_t *outputBuffer = malloc(totalSize);
    if (!outputBuffer) return -1;
    if (verbose) printf("Done\n");

    size_t offset = 0;

    // header
    if (verbose) printf("Allocating header space... ");
    for (int i = 0; i < 7; i++) {
      memcpy(outputBuffer + offset, &header[i], sizeof(uint32_t));
      offset += sizeof(uint32_t);
    }
    if (verbose) printf("Done\n");

    // file data
    if (verbose) printf("Allocating file data space... ");
    memcpy(outputBuffer + offset, fileData, fileSize);
    offset += fileSize;
    if (verbose) printf("Done\n");

    // crc32
    if (verbose) printf("Allocating checksum space... ");
    memcpy(outputBuffer + offset, &checksum, sizeof(uint32_t));
    if (verbose) printf("Done\n");

    // Convert bits to numbers
    // Allocate frequency according to chosen value
    uint16_t actualFrequency = 0;
    switch (bitFrequency)
    {
      case 0:
        // Extra Low
        actualFrequency = 100;
        break;
      case 1:
        // Low
        actualFrequency = 400;
        break;
      case 2:
        // Medium
        actualFrequency = 1000;
        break;
      case 3:
        // High
        actualFrequency = 2000;
        break;
      case 4:
        // Really high
        actualFrequency = 8000;
        break;
      case 5:
        // Extremely high
        actualFrequency = 22000;
        break;
      default:
        // Default value is 1000 / medium
        actualFrequency = 1000;
        break;
    }
    // Allocate space for each frequency
    size_t expandedSize = totalSize * 8;
    if (verbose) printf("Allocating frequency space... ");
    uint16_t *expandedBuffer = malloc(expandedSize * sizeof(uint16_t));
    expandBitsToFreq(outputBuffer, totalSize, expandedBuffer, actualFrequency);
    if (verbose) printf("Done\n");

    // Convert numbers to tones + write file
    int actualBitrate = 200;
    switch (bitrate)
    {
      case 0:
        // Extra Low
        actualBitrate = 1;
        break;
      case 1:
        // Low
        actualBitrate = 10;
        break;
      case 2:
        // Medium
        actualBitrate = 200;
        break;
      case 3:
        // High
        actualBitrate = 500;
        break;
      case 4:
        // Really high
        actualBitrate = 1000;
        break;
      case 5:
        // Extremely high
        actualBitrate = 2500;
        break;
      default:
        // Default value is 200 / medium
        actualBitrate = 200;
        break;
    }

    int actualSamplerate = 0;
    // Just a quick rant here:
    // We can only hear up to 20000Hz right and
    // the maximum frequency that can be played
    // correcly by an audio source is its
    // samplerate halved. CD has a sample rate
    // of 44100Hz. Divide that by 2 and you
    // get 22050. So why is CD seen as low
    // quality when it can literally hold
    // every single frequency we can hear and
    // more (for some lee-way). Marketing.
    // That's why...
    switch (sampleRate)
    {
      case 0:
        // Telephone
        actualSamplerate = 8000;
        break;
      case 1:
        // Subwoofer
        actualSamplerate = 11025;
        break;
      case 2:
        // VOIP
        actualSamplerate = 16000;
        break;
      case 3:
        // MPEG
        actualSamplerate = 22050;
        break;
      case 4:
        // CD
        actualSamplerate = 44100;
        break;
      case 5:
        // DVD
        actualSamplerate = 48000;
        break;
      case 6:
        // Mastering
        actualSamplerate = 88200;
        break;
      case 7:
        // Blu-Ray
        actualSamplerate = 96000;
        break;
      case 8:
        // HDCD
        actualSamplerate = 176400;
        break;
      case 9:
        // "Professional"
        actualSamplerate = 192000;
        break;
      case 10:
        // DxD (lmfao)
        actualSamplerate = 352800;
        break;
      case 11:
        // maximum PCM Frequency
        actualSamplerate = 384000;
        break;
      default:
        // Default value is 44100 / CD quality
        actualSamplerate = 44100;
        break;
    }


    size_t numSamples;
    if (verbose) printf("Writing file... ");
    uint16_t *pcmSamples = generateAudioSamples(expandedBuffer, expandedSize, actualSamplerate, actualBitrate, &numSamples);
    writeWAV(outputFile, pcmSamples, numSamples, actualSamplerate);
    if (verbose) printf("Done\n");
    free(pcmSamples);

    // Clean up
    free(outputBuffer);
    free(expandedBuffer);

  } //-----------------------------------------------------------------------------------------------------------------------

  // Clean up

  free(fileData);

  return 0;
}
