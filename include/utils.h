#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

// A function that takes the contents of // a selected file from ./text/* and 
// outputs it to the screen.
// Takes in 'w' for WARRANTY
// Takes in 'v' for VERSION
// Takes in 'h' for HELP
// Returns -1 if file not found.
int
displayInfo(char opt);

// A function that format all inputted filenames into the standard DOS 8.3 format
// All uppercase and with no separating dot.
int
formatFilename(char *fileNameIn);
void
unformatFilename(const char dos[11], char *out);

// A utility to create a header for use by cassave
int
createHeader(uint32_t header[7], char *filename, uint64_t filesize, uint8_t compressionFlag);

// A utility to parse the header given by cassave
int
parseHeader(uint32_t header[5], char filename[12], uint64_t *filesize, uint8_t *compressionFlag, uint8_t *version);

// A utility to detect whether a file is PCM WAV or not
int
isWavFile(const char *filename);

// A set of utilities for finding the CRC32 sum of a file provided.
uint32_t
crc32_update(uint32_t crc, uint8_t data);
uint32_t
crc32_create(uint8_t *data, size_t fileSize);

// A utility for finding length of file (POSIX only)
int
getFileSize(const char *filename, uint64_t *size);

int
importFile(char *filename, uint8_t **fileData, uint64_t *fileSize);

void
expandBitsToFreq(const uint8_t *outputBuffer, size_t outputLen, uint16_t *expandedBuffer, uint16_t actualFrequency);

int
writeWAV(const char *filename, const uint16_t *samples, size_t numSamples, uint32_t sampleRate);
uint16_t*
generateAudioSamples(const uint16_t *expandedBuffer, size_t numBits, uint32_t sampleRate, uint32_t bitrate, size_t *outSampleCount);
double
goertzel(const int16_t *samples, size_t n, double targetFreq, double sampleRate);
size_t
decodeBitsFromPCM(const int16_t *pcm, size_t totalSamples, uint32_t sampleRate, uint32_t bitrate, uint16_t baseFreq, uint8_t **outBits);
size_t
bitsToBytes(const uint8_t *bits, size_t numBits, uint8_t **outBytes);
int
readWavPCM16(const char *filename, int16_t **samples, size_t *numSamples, uint32_t *sampleRate);
int
decodeCassaveWav(const char *wavFile, uint32_t bitrate, uint16_t baseFreq);

//DEV
void
dump_hex(const uint8_t *buf, size_t len);
void
dump_hex16(const uint16_t *buf, const uint8_t *buf2, size_t len);
void
print_buffer_decimal(const uint8_t *buf, size_t len);
void
print_buffer_bits(const uint8_t *buf, size_t len);

#endif
