/*
 * cassave utils
 * Copyright (C) 2025 - 2026 Toby Valentine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the license, or
 * (at your option) any later version.*/

#include "../include/raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "../include/raygui.h"
#include "../include/style_amber.h"
#include "../include/utils.h"

int verbose = false;

// let simple numbers be used to describe windows
enum ProgramWindow {
  NONE_MENU = 0,
  HELP_MENU,
  INFO_MENU,
  SETTING_MENU
};

int main(void)
{
  // Initialise window of 860x640px with a title
  InitWindow(860, 640, "cassave - Data to Audio Converter");
  
  // Initialise amber style
  GuiLoadStyleAmber();


  // Don't need insane frames per second, 60 will give good performance
  // with smooth input
  SetTargetFPS(60);

  // additional menus should be closed by default
  uint8_t windowShow = NONE_MENU;
  
  // dropdown menus should have defaults
  int compressionAlgorithm = 1; // LZMA level 5
  int bitrate = 2;
  int bitFrequencies = 2;
  int sampleRate = 4;
    
  // where text inputs are stored
  char fileInput[260] = {"\0"}; // maximum file length for most systems
  char fileOutput[15] = {"\0"}; // max file length is 11, space for padding
  
  // if text boxes are currently used
  bool fileInputEditMode = false;
  bool fileOutputEditMode = false;
  int textDisplay = 0;

  // Only close the window if told to
  while (!WindowShouldClose())
  {
    // Draw the window
    BeginDrawing();
      ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));



      if (GuiButton((Rectangle){2, 2, 50, 20}, "#193#Help"))
      {
        windowShow = HELP_MENU;
      } else if (GuiButton((Rectangle){54, 2, 50, 20}, "#191#Info")) 
      {
        windowShow = INFO_MENU;
      } else if (GuiButton((Rectangle){106, 2, 75, 20}, "#22#Settings"))
      {
        windowShow = SETTING_MENU;
      }

      if (windowShow == HELP_MENU)
      {
        int result = GuiMessageBox((Rectangle){50, 50, 250, 120},
            "#193#Help", "Help can be found in README.MD", "Okay");

        if (result >= 0) windowShow = NONE_MENU;

      } else if (windowShow == INFO_MENU)
      {
        int result = GuiMessageBox((Rectangle){50, 50, 250, 200},
            "#191#Info", "Copyright (C) 2025 Toby Valentine.\nLicensed under GNU" 
            " GPLv3.\nSee COPYING for more information", "Okay");

        if (result >= 0) windowShow = NONE_MENU;
      
      } else if (windowShow == SETTING_MENU)
      {
        int result = GuiMessageBox((Rectangle){50, 50, 400, 300},
            "#22#Settings", "", "Okay");
        GuiLabel((Rectangle){70, 80, 380, 40}, "You probably won't need to change these "
            "settings, but\nthey're here in case you do");

        // Compression is not yet included
        // GuiLabel((Rectangle){70, 140, 150, 20}, "Compression Algorithm");
        // GuiComboBox((Rectangle){70, 160, 250, 20}, "None;LZMA (Better Performance);LZMA (Smaller Size);BZIP2 (Encrypt)",
        //     &compressionAlgorithm);
    
        GuiLabel((Rectangle){70, 140, 250, 20}, "Bit Rate");
        GuiLabel((Rectangle){70, 180, 250, 20}, "Tone Frequencies");
        GuiLabel((Rectangle){70, 220, 250, 20}, "Sample Rate");
        GuiComboBox((Rectangle){70, 160, 250, 20}, "Extra Low (1bps);Low (10bps)"
        ";Medium (200bps);High (500bps);Really High (1000bps);Extremely High "
        "(5000bps)", &bitrate);

        GuiComboBox((Rectangle){70, 200, 250, 20}, "Extra Low (100/200Hz)"
        ";Low (400/800Hz);Medium (1000/2000Hz);High (2000/4000Hz);Really High "
        "(8000/16000Hz); Extremely High (22000/44000Hz)",
                    &bitFrequencies);

        GuiComboBox((Rectangle){70, 240, 250, 20}, "Telephone (8000Hz);"
        "Subwoofer (11025Hz);VOIP (16000Hz);MPEG (22050Hz);CD (44100Hz);"
        "DVD (48000Hz);Mastering (88200Hz);Blu-Ray (96000Hz);HDCD (176400Hz);"
        "Professional (192000Hz);DXD (352800Hz);"
        "Maximum (384000Hz)", &sampleRate);

        // Settings save is not yet included
        // if (GuiButton((Rectangle){340, 240, 75, 20}, "#6#Save All"))
        // {
        //   // Create .cfg file for these settings with json.
        // }

        if (result >= 0) windowShow = NONE_MENU;
      } else
      {
        GuiLabel((Rectangle){20, 30, 100, 20}, "Input file:");
        GuiLabel((Rectangle){20, 70, 100, 20}, "Output file:");
        if (GuiTextBox((Rectangle){20, 50, 100, 20}, fileInput, 260, fileInputEditMode))
        {
          fileInputEditMode = !fileInputEditMode;
        } else if (GuiTextBox((Rectangle){20, 90, 100, 20}, fileOutput, 15,
                    fileOutputEditMode))
        {
          fileOutputEditMode = !fileOutputEditMode;
        }
        switch (textDisplay)
        {
          case 0:
            GuiLabel((Rectangle){20, 150, 110, 20}, "                    ");
            break;
          case 1:
            GuiLabel((Rectangle){20, 150, 110, 20}, "File not found      ");
            break;
          case 2:
            GuiLabel((Rectangle){20, 150, 110, 20}, "Decoding            ");
            break;
          case 3:
            GuiLabel((Rectangle){20, 150, 110, 20}, "Done                ");
            break;
          case 4:
            GuiLabel((Rectangle){20, 150, 110, 20}, "Encoding            ");
            break;

        }

        if (GuiButton((Rectangle){20, 130, 110, 20}, "Process!"))
        {
          textDisplay = 0;
          // File to be input
          uint8_t *fileData = NULL;
          // Where the headers are stored
          uint32_t header[7] = {0};
          // 64 Bit file size (large maximum size
          size_t fileSize;
          // CRC32 is 32 bit (hence the name)
          uint32_t checksum;

          if (isWavFile(fileInput) == -1) // File doesn't exist---------------------------------------------------------------------
          {
            GuiLabel((Rectangle){20, 150, 110, 20}, "File not found.");
            textDisplay = 1;

          } else if (isWavFile(fileInput)) // File is WAV-----------------------------------------------------------------------------
          {
            // Decode
            textDisplay = 2;

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
            switch (bitFrequencies)
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

            decodeCassaveWav(fileInput, actualBitrate, actualFrequency);

            textDisplay = 3;


          } else // File is not WAV------------------------------------------------------------------------------------------------------
          {
            // Encode

            textDisplay = 4;

            if (importFile(fileInput, &fileData, &fileSize) != 0)
            {
              fprintf(stderr, "Failed to import file\n");
              return -1;
            }

            // Only format basename (not path)
            char newFileName[12];
            snprintf(newFileName, 12, fileInput);
            char *base = strrchr(newFileName, '/');
            base = base ? base + 1 : newFileName;
            formatFilename(base);

            getFileSize(fileInput, &fileSize);

            createHeader(header, base, fileSize, compressionAlgorithm);

            // Make cksum
            checksum = crc32_create(fileData, fileSize);

            // Append header + binary data + crc
            // Find sizes of everything
            size_t headerBytes = 7 * sizeof(uint32_t);
            size_t crcBytes    = sizeof(uint32_t);
            size_t totalSize = headerBytes + fileSize + crcBytes;

            // Allocate the memory
            uint8_t *outputBuffer = malloc(totalSize);
            if (!outputBuffer) return -1;

            size_t offset = 0;

            // header
            for (int i = 0; i < 7; i++) {
              memcpy(outputBuffer + offset, &header[i], sizeof(uint32_t));
              offset += sizeof(uint32_t);
            }

            // file data
            memcpy(outputBuffer + offset, fileData, fileSize);
            offset += fileSize;

            // crc32
            memcpy(outputBuffer + offset, &checksum, sizeof(uint32_t));

            // Convert bits to numbers
            // Allocate frequency according to chosen value
            uint16_t actualFrequency = 0;
            switch (bitFrequencies)
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
            uint16_t *expandedBuffer = malloc(expandedSize * sizeof(uint16_t));
            expandBitsToFreq(outputBuffer, totalSize, expandedBuffer, actualFrequency);

            // Convert numbers to tones + write file
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

            int actualSamplerate = 0;
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
            uint16_t *pcmSamples = generateAudioSamples(expandedBuffer, expandedSize, actualSamplerate, actualBitrate, &numSamples);
            writeWAV(fileOutput, pcmSamples, numSamples, actualSamplerate);
            free(pcmSamples);

            // Clean up
            free(outputBuffer);
            free(expandedBuffer);

            textDisplay = 3;
          }

        }

      }
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
