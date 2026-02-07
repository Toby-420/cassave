/*
 * cassave utils
 * Copyright (C) 2025 - 2026 Toby Valentine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the license, or
 * (at your option) any later version.*/

#include <stdio.h>
#include <string.h>
#include <libintl.h>
#include <locale.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <sndfile.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>

#define CASSAVE_VERSION 1
#define TOLERANCE 50

// Common shorthand for string localisation
#define _(str) gettext(str)

// Stuff for WAVs
#pragma pack(push,1) // Ensure no padding
typedef struct {
  char     chunkID[4];      // "RIFF"
  uint32_t chunkSize;
  char     format[4];       // "WAVE"

  // fmt subchunk
  char     subchunk1ID[4];  // "fmt "
  uint32_t subchunk1Size;   // 16 for PCM
  uint16_t audioFormat;     // 1 = PCM
  uint16_t numChannels;     // 1 = mono, 2 = stereo
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;

  // data subchunk
  char     subchunk2ID[4];  // "data"
  uint32_t subchunk2Size;   // numSamples * numChannels * bitsPerSample/8
} WAVHeader;
#pragma pack(pop)

// Acquire veriables from other files
extern int verbose;

// ------------------------------------------------------------
// A utility to display the information stored
// in the ./text/* files.
int 
displayInfo (int toDisplay) {
  // Need file pointer and somewhere to store
  // the contents
  FILE *file;
  int bufferSize;
  char fileToOpen[20];
  switch (toDisplay) {
    case 1:
      bufferSize = 100;
      snprintf (fileToOpen, 20, "./text/HELP");
      break;
    case 2:
      bufferSize = 8;
      snprintf(fileToOpen, 20, "./text/VERSION");
      break;
    case 3:
      bufferSize = 100;
      snprintf (fileToOpen, 20, "./text/WARRANTY");
      break;
    default:
      bufferSize = 0;
      snprintf (fileToOpen, 20, "./text/HELP");
      break;
  }
  char fileContents[bufferSize];
  // Open file in read mode
  file = fopen (fileToOpen, "r");
  
  // Check if the file exists
  if (file == NULL) {
    if (verbose) {
      printf ("%s was not found.", fileToOpen);
    }
    return -1;
  } else {
    // Acquire the contents into fileContents
    while (fgets(fileContents, bufferSize, file)) {
      printf ("%s", fileContents);
    }
  }
  
  // Output and close the file, return success
  fclose (file);
  return 0;
}
// ------------------------------------------------
// A utility to convert filenames into the DOS 8.3
// format for constant length.
// Takes in filename as char*
int
formatFilename(char *fileNameIn)
{
  if (fileNameIn == NULL)
    return -1;

  char name[8];
  char ext[3];

  // Fill with spaces
  memset(name, ' ', sizeof(name));
  memset(ext,  ' ', sizeof(ext));

  char *dot = strchr(fileNameIn, '.');

  // Copy filename (before dot) with tertiary operator
  size_t nameLen = dot ? (size_t)(dot - fileNameIn) : strlen(fileNameIn);
  if (nameLen > 8)
  {
    nameLen = 8;
  }

  for (size_t i = 0; i < nameLen; i++)
  {
    name[i] = toupper((unsigned char)fileNameIn[i]);
  }

  // Copy extension (after dot)
  if (dot != NULL) {
    size_t extLen = strlen(dot + 1);
    if (extLen > 3)
    {
      extLen = 3;
    }

    for (size_t i = 0; i < extLen; i++)
    {
      ext[i] = toupper((unsigned char)dot[1 + i]);
    }
  }

  // Write result back (8 + 3 + null terminator)
  memcpy(fileNameIn, name, 8);
  memcpy(fileNameIn + 8, ext, 3);
  fileNameIn[11] = '\0';

  return 0;
}

// Just reverses the above process
// Convert DOS 8.3 (space-padded, 11 bytes) into normal filename
// out must be at least 13 bytes
void unformatFilename(const char dos[11], char *out)
{
  int pos = 0;

  // filename part (0–7)
  for (int i = 0; i < 8; i++) {
    if (dos[i] == ' ')
      break;
    out[pos++] = dos[i];
  }

  // extension part (8–10)
  bool hasExt = false;
  for (int i = 8; i < 11; i++) {
    if (dos[i] != ' ') {
      hasExt = true;
      break;
    }
  }

  if (hasExt) {
    out[pos++] = '.';
    for (int i = 8; i < 11; i++) {
      if (dos[i] == ' ')
        break;
      out[pos++] = dos[i];
    }
  }

  out[pos] = '\0';
}



//------------------------------------------------
// HEADER LAYOUT:
//
// preamble...  (32 bit)
// magic number (0xDEADBEEF, 32 bit)
// flags byte   (8 bit (incompatible versions i.e. framework versions,
//                        not software versions). also contains
//                        compression/encryption flag)
// filename     (88 bit in 8.3 format)
// file size    (64 bit unsigned int)
// Total length:  224 bit (28 bytes)
// Useful length: 192 bit (24 bytes)
//-----------------------------------------------
// A utility to create the header for the data to be
// encoded in cassave
// Takes in filename (formatted), file size, compression
// flag, and reference to where the data goes.
int
createHeader(uint32_t header[7], char *filename, uint64_t filesize,
             uint8_t compressionFlag)
{
  int preamble = 0x55555555; // 01010101 repeated 4 times
  int magicNum = 0xDEADBEEF; // Highly unlikely to naturally appear

  // Concatenate the compression flag and version into one byte.
  // This is for efficiency so there isn't a single bit hanging around.
  // This lets there be 4 modes for compression: none, lzma (level 5), deflate (level 9), bzip2 (encryption)
  uint8_t flagByte = (compressionFlag & 0b00000011) << 6 | CASSAVE_VERSION;

  // Just a bit of debug info using int types
  if (verbose)
  {
    printf("Version.: %"PRIu8"\nCompression:%"PRIu8"\nFilename: %s\nFilesize: %"PRIu64"\n",
           (flagByte & 0b00111111), ((flagByte & 0b11000000) >> 6), filename, filesize);
  }
  // Concatenate all the data into one long. Uses bitwise *magic*
  for (int i = 0; i < 7; i++)
  {
    switch (i)
    {
      // 0 to 31
      case 0:
        if (verbose) printf("Assigning preamble bits... ");
        header[i] = (uint32_t)preamble;
        if (verbose) printf("Done\n");
        break;


        // 32 to 63
      case 1:
        if (verbose) printf("Assigning magic number bits... ");
        header[i] = (uint32_t)magicNum;
        if (verbose) printf("Done\n");
        break;

        // 64 to 95
        // just a bunch of shifting bits from memory addresses.
        // be glad there's no pointer arithmetic
      case 2:
        if (verbose) printf("Assigning flag byte and filename chars 0 to 2... ");
        header[i] = ((uint32_t)flagByte << 24) | ((uint32_t)filename[0] << 16)
        | ((uint32_t)filename[1] << 8) | (uint32_t)filename[2];
        if (verbose) printf("Done\n");
        break;

        // 96 to 127
        // middle parts of filename, just shifting and masking (bitwire OR |)
      case 3:
        if (verbose) printf("Assigning filename chars 3 to 6... ");
        header[i] = ((uint32_t)filename[3] << 24) | ((uint32_t)filename[4] << 16)
        | ((uint32_t)filename[5] << 8)  | (uint32_t)filename[6];
        if (verbose) printf("Done\n");
        break;

        // 128 to 159
        // Add the last parts of the filename
      case 4:
        if (verbose) printf("Assigning filename chars 7 to 10... ");
        header[i] = ((uint32_t)filename[7] << 24) | ((uint32_t)filename[8] << 16)
        | ((uint32_t)filename[9] << 8)  | (uint32_t)filename[10];
        if (verbose) printf("Done\n");
        break;

        // 160 to 191
        // shift down, convert to uint32_t.
        // this just discards the lower 32 bits to give us
        // the required higher bits.
      case 5:
        if (verbose) printf("Assigning upper 32 bits of file size... ");
        header[i] = (uint32_t)(filesize >> 32);
        if (verbose) printf("Done\n");
        break;

        // 192 to 223
        // effectively the same as before, except we mask
        // instead of shifting to give us the lower 32 bits
        // and discard the top 32 bits.
      case 6:
        if (verbose) printf("Assigning lower 32 bits of file size... ");
        header[i] = (uint32_t)(filesize & 0xFFFFFFFF);
        if (verbose) printf("Done\n");
        break;

      default:
        // This will never happen unless the computer goes seriously wrong.
        printf("Oh no. Something really went wrong here.\n");
        return -1;
    }
  }

  return 0;
}
//-----------------------------------------------
// A utility to parse the header created by the above function.
// Takes in header data (ignoring preamble and magic number),
//    filename reference, file size reference, and compressionFlag reference.
int
parseHeader(uint32_t header[5], char filename[12], uint64_t *filesize, uint8_t *compressionFlag, uint8_t *version)
{

  if (header == NULL)
  {
    printf("Header is NULL.\n");
    return -1;
  }
  // extract the contents of what was previously put into the header
  if (verbose) printf("Extracting version from header... ");
  *version = (((header[0] >> 24) & 0b00111111));
  if (verbose) printf("Done\n");

  if (verbose) printf("Extracting compression mode from header... ");
  // set the value in the memory location (dereference pointer) rather than local copy
  *compressionFlag = (header[0] >> 30) & 0b11;
  if (verbose) printf("Done\n");

  // iterate over for the filename
  // this is the opposite of the encoding function
  if (verbose) printf("Extracting filename from header... ");
  for (int i = 0; i < 11; i++)
  {
    switch (i) {
      case 0 ... 2:
        filename[i] = (header[0] >> (8 * (2 - i))) & 0b11111111;  // Extract from header[0]
        break;
      case 3 ... 6:
        filename[i] = (header[1] >> (8 * (6 - i))) & 0b11111111;  // Extract from header[1]
        break;
      case 7 ... 10:
        filename[i] = (header[2] >> (8 * (10 - i))) & 0b11111111; // Extract from header[2]
        break;
      default:
        break;
    }
  }
  if (verbose) printf("Done\n");

  // Apply the filesize to the memory location provided
  if (verbose) printf("Extracting filesize from header... ");
  *filesize = ((uint64_t)header[3] << 32) | (uint64_t)header[4];
  if (verbose) printf("Done\n");

  return 0;
}

// A utility to read in the file and store it as bits.
// Very simple idea, just needs to be as efficient as possible.
int importFile(const char *filename, uint8_t **fileData, uint64_t *fileSize)
{
  FILE *file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "%s was not found.\n", filename);
    return -1;
  }

  // Seek to end to get size
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return -1;
  }

  uint32_t size = ftell(file);
  if (size < 0) {
    fclose(file);
    return -1;
  }

  // Send the file pointer back to the start
  rewind(file);

  // Allocate buffer
  uint8_t *buffer = malloc((size_t)size);
  if (!buffer) {
    fclose(file);
    return -1;
  }

  // Read entire file
  size_t bytesRead = fread(buffer, 1, (size_t)size, file);
  fclose(file);

  if (bytesRead != (size_t)size) {
    free(buffer);
    return -1;
  }

  *fileData = buffer;
  *fileSize = (uint64_t)size;

  return 0;
}


// A utility to detect whether a file is a PCM WAV file or not.
// Will return 1 if correct, 0 if not.
int isWavFile(const char *filename)
{
  FILE *f = fopen(filename, "rb");
  if (!f)
  {
    return -1;
  }

  unsigned char header[12];

  if (fread(header, 1, 12, f) != 12) {
    fclose(f);
    return 0;
  }

  fclose(f);

  return memcmp(header, "RIFF", 4) == 0 &&
         memcmp(header + 8, "WAVE", 4) == 0;
}

// A set of utilities for finding the CRC32 sum of a file provided.
uint32_t crc32_update(uint32_t crc, uint8_t data)
{
  crc ^= data;
  for (int i = 0; i < 8; i++)
    crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
  return crc;
}

uint32_t crc32_create(uint8_t *data, size_t len)
{
  uint32_t crc = 0xFFFFFFFF;

  for (size_t i = 0; i < len; i++) {
    crc = crc32_update(crc, data[i]);
  }

  return crc ^ 0xFFFFFFFF;
}

// A utiliy for finding file length (POSIX, not ISO C)
int getFileSize(const char *filename, size_t *size)
{
  struct stat st;
  if (stat(filename, &st) != 0)
    return -1;

  *size = (size_t)st.st_size;
  return 0;
}

// Function to expand data into PCM samples
void expandBitsToFreq(const uint8_t *outputBuffer, size_t outputLen,
                      uint16_t *expandedBuffer, uint16_t actualFrequency)
{
  size_t expandedIndex = 0;

  for (size_t i = 0; i < outputLen; i++) {
    uint8_t byte = outputBuffer[i];

    // Process each bit, from MSB to LSB
    for (int bit = 7; bit >= 0; bit--) {
      uint16_t value = (byte & (1 << bit)) ? (actualFrequency * 2) : actualFrequency;
      expandedBuffer[expandedIndex++] = value;
    }
  }
}

int writeWAV(const char *filename, const uint16_t *samples, size_t numSamples,
             uint32_t sampleRate)
{
  WAVHeader header;

  // Fill RIFF header
  memcpy(header.chunkID, "RIFF", 4);
  memcpy(header.format, "WAVE", 4);

  // fmt subchunk
  memcpy(header.subchunk1ID, "fmt ", 4);
  header.subchunk1Size = 16;
  header.audioFormat = 1; // PCM
  header.numChannels = 1; // mono
  header.sampleRate = sampleRate;
  header.bitsPerSample = 16;
  header.blockAlign = header.numChannels * header.bitsPerSample / 8;
  header.byteRate = header.sampleRate * header.blockAlign;

  // data subchunk
  memcpy(header.subchunk2ID, "data", 4);
  header.subchunk2Size = numSamples * header.numChannels * header.bitsPerSample / 8;

  // total RIFF size
  header.chunkSize = 36 + header.subchunk2Size;

  FILE *f = fopen(filename, "wb");
  if (!f) {
    perror("fopen");
    return -1;
  }

  // Write header
  fwrite(&header, sizeof(WAVHeader), 1, f);

  // Write samples
  fwrite(samples, sizeof(uint16_t), numSamples, f);

  fclose(f);
  return 0;
}
uint16_t* generateAudioSamples(const uint16_t *expandedBuffer, size_t numBits,
                               uint32_t sampleRate, uint32_t bitrate, size_t *outSampleCount)
{
  size_t samplesPerBit = sampleRate / bitrate;
  *outSampleCount = numBits * samplesPerBit;

  uint16_t *pcm = malloc(*outSampleCount * sizeof(uint16_t));
  if (!pcm) return NULL;

  for (size_t i = 0; i < numBits; i++) {
    uint16_t freq = expandedBuffer[i];
    for (size_t s = 0; s < samplesPerBit; s++) {
      // Generate sine wave
      double t = (double)s / sampleRate;
      double val = sin(2.0 * M_PI * freq * t) * 30000.0;
      pcm[i * samplesPerBit + s] = (int16_t)val;

    }
  }

  return pcm;
}

double goertzel(const int16_t *samples, size_t n,
                       double targetFreq, double sampleRate)
{
  double s_prev = 0.0, s_prev2 = 0.0;
  double normalizedFreq = targetFreq / sampleRate;
  double coeff = 2.0 * cos(2.0 * M_PI * normalizedFreq);

  for (size_t i = 0; i < n; i++) {
    double s = samples[i] + coeff * s_prev - s_prev2;
    s_prev2 = s_prev;
    s_prev = s;
  }

  return s_prev2 * s_prev2 + s_prev * s_prev - coeff * s_prev * s_prev2;
}
size_t decodeBitsFromPCM(const int16_t *pcm, size_t totalSamples,
                         uint32_t sampleRate, uint32_t bitrate,
                         uint16_t baseFreq,
                         uint8_t **outBits)
{
  size_t samplesPerBit = sampleRate / bitrate;
  size_t numBits = totalSamples / samplesPerBit;

  uint8_t *bits = malloc(numBits);
  if (!bits) return 0;

  for (size_t i = 0; i < numBits; i++) {
    const int16_t *chunk = pcm + i * samplesPerBit;

    double e0 = goertzel(chunk, samplesPerBit,
                         baseFreq, sampleRate);
    double e1 = goertzel(chunk, samplesPerBit,
                         baseFreq * 2, sampleRate);

    bits[i] = (e1 > e0) ? 1 : 0;
  }

  *outBits = bits;
  return numBits;
}
size_t bitsToBytes(const uint8_t *bits, size_t numBits,
                   uint8_t **outBytes)
{
  size_t numBytes = numBits / 8;
  uint8_t *bytes = calloc(numBytes, 1);
  if (!bytes) return 0;

  for (size_t i = 0; i < numBytes; i++) {
    uint8_t b = 0;
    for (int j = 0; j < 8; j++) {
      b <<= 1;
      b |= bits[i * 8 + j];
    }
    bytes[i] = b;
  }

  *outBytes = bytes;
  return numBytes;
}
int readWavPCM16(const char *filename,
                 int16_t **samples,
                 size_t *numSamples,
                 uint32_t *sampleRate)
{
  SF_INFO info = {0};
  SNDFILE *sf = sf_open(filename, SFM_READ, &info);
  if (!sf) return -1;

  if (info.channels != 1 || info.format != (SF_FORMAT_WAV | SF_FORMAT_PCM_16)) {
    sf_close(sf);
    return -1;
  }

  int16_t *buf = malloc(info.frames * sizeof(int16_t));
  sf_readf_short(sf, buf, info.frames);

  *samples = buf;
  *numSamples = info.frames;
  *sampleRate = info.samplerate;

  sf_close(sf);
  return 0;
}
int decodeCassaveWav(const char *wavFile,
                     uint32_t bitrate,
                     uint16_t baseFreq)
{
  int16_t *pcm;
  size_t samples;
  uint32_t sr;

  if (readWavPCM16(wavFile, &pcm, &samples, &sr) != 0)
    return -1;

  uint8_t *bits;
  size_t numBits = decodeBitsFromPCM(
    pcm, samples, sr, bitrate, baseFreq, &bits
  );

  uint8_t *bytes;
  size_t numBytes = bitsToBytes(bits, numBits, &bytes);

  /* ---- parse header ---- */
  if (verbose) printf("Parsing Header... ");
  uint32_t headerRaw[7];
  memcpy(headerRaw, bytes, 28);

  uint32_t headerDecoded[5];
  memcpy(headerDecoded, &headerRaw[2], 20);
  if (verbose) printf("Done\n");

  /* 11 bytes for DOS name */
  if (verbose) printf("Formatting filename... ");
  char dosFilename[12];
  memset(dosFilename, ' ', sizeof(dosFilename));

  char realFilename[13] = {0};

  uint64_t filesize;
  uint8_t compression, version;

  parseHeader(headerDecoded, dosFilename,
              &filesize, &compression, &version);

  unformatFilename(dosFilename, realFilename);
  if (verbose) printf("Done\n");


  if (verbose) printf("Comparing checksums... ");
  uint8_t *payload = bytes + 28;
  uint32_t storedCRC;
  memcpy(&storedCRC, payload + filesize, 4);

  uint32_t calcCRC = crc32_create(payload, filesize);

  if (storedCRC != calcCRC) {
    fprintf(stderr, "CRC mismatch\n");
    return -1;
  }
  if (verbose) printf("Done\n");

  if (verbose) printf("Writing file... ");
  FILE *out = fopen(realFilename, "wb");
  fwrite(payload, 1, filesize, out);
  fclose(out);
  if (verbose) printf("Done\n");

  free(pcm);
  free(bits);
  free(bytes);

  return 0;
}


/* -------------------------------------------------------------------------------
//
//
//                      ,-,-
//                     / / |
//   ,-'             _/ / /
//  (-_          _,-' `Z_/
//   "#:      ,-'_,-.    \  _
//    #'    _(_-'_()\     \" |      |  | |-- |-\ |--  |-\ |--
//  ,--_,--'                 |      |--| |-- |_| |--  |-. |--
// / ""                      L-'\   |  | |-- | \ |--  |-/ |--
// \,--^---v--v-._        /   \ |
//  \_________________,-'      |    |-\ |-\ /-\ /-\ /-\ |\| /-
//                   \              | | |_| |-| |_/ | | ||| |_
//                    \             |-/ | \ | | \_. \-/ | \  /
// Wny                 \\
//
//
// None of the code beneath has been tested or made readable, it is
// purely for testing and debugging purposes
// -----------------------------------------------------------------------------*/

void dump_hex(const uint8_t *buf, size_t len)
{
  for (size_t i = 0; i < len; i++) {
    if (i % 16 == 0)
      printf("%08zx  ", i);

    printf("%02X ", buf[i]);

    if (i % 16 == 15 || i == len - 1)
      printf("\n");
  }
}
void dump_hex16(const uint16_t *buf, const uint8_t *buf2, size_t len)
{
  int j  = 0;
  for (size_t i = 0; i < len; i++) {
    // Print address at the start of each line (16 words per line)
    if (i % 16 == 0)
    {
      printf("%02X %02X ", buf2[j], buf2[j+1]); // multiply by 2 for byte offset
      j+=2;
    }
      // Print each 16-bit value as 4 hex digits
    printf("%04X ", buf[i]);

    // End of line
    if (i % 16 == 15 || i == len - 1)
      printf("\n");
  }
}
void print_buffer_decimal(const uint8_t *buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    printf("%u ", buf[i]);  // print each byte as 0–255
    if ((i + 1) % 16 == 0)
      putchar('\n');     // optional: newline every 16 bytes
  }
  putchar('\n');
}
void print_byte_bits(uint8_t b) {
  for (int i = 7; i >= 0; i--) {  // MSB first
    putchar((b & (1 << i)) ? '1' : '0');
  }
}

void print_buffer_bits(const uint8_t *buf, size_t len) {
  for (size_t i = 0; i < len; i++) {
    print_byte_bits(buf[i]);
    putchar(' ');  // separate bytes
    if ((i + 1) % 8 == 0)
      putchar('\n');  // optional: new line every 8 bytes
  }
  putchar('\n');
}
