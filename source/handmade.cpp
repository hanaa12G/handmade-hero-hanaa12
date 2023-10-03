#include "handmade.hpp"
#include <cstdint>

#define internal_func static
#define local_persist static
#define global_variable static


#define PI 3.14159265359f



internal_func void
GameOutputSound(game_sound_output_buffer* SoundOutputBuffer, unsigned ToneHz)
{
  local_persist float Angle = 0.0f;
  local_persist unsigned Volume = 4000;

  unsigned SampleCountToOutput = SoundOutputBuffer->SampleCountToOutput;
  int16_t* Samples = SoundOutputBuffer->Samples;
  float    WavePeriod = (float) SoundOutputBuffer->SamplesPerSecond / (float) ToneHz;



  float AngleStep = (2 * PI / (float) WavePeriod);

  for (int SampleIndex = 0;
      SampleIndex < SampleCountToOutput;
      ++SampleIndex)
  {
    float t = Angle; 

    int16_t SampleValue = (int16_t) (sinf(t) * Volume);

    *Samples++ = SampleValue;

    Angle += AngleStep;
    if (Angle > 2 * PI) {
      Angle -= 2 * PI;
    }
  }
}

internal_func void
RenderWeirdRectangle(game_offscreen_buffer* Buffer, int XOffset, int YOffset)
{
  uint8_t* Rows = (uint8_t*) Buffer->Data;
  for (int Y = 0;
       Y < Buffer->Height;
       ++Y)
  {
    uint32_t* Pixel = (uint32_t*) Rows;
    for (int X = 0;
         X < Buffer->Width;
         ++X)
    {
      /*
      Memory: RR GG BB xx
      Register: xx RR GG BB
      */

      uint8_t Green = (uint8_t) (XOffset + X);
      uint8_t Blue  = (uint8_t) (YOffset + Y);

      *Pixel++ = (uint32_t)((Green << 8) | Blue);

    }

    // Rows += BytesPerPixel * Buffer->Width;
    Rows += Buffer->Pitch;
  }
}

void GameUpdateAndRender(game_offscreen_buffer* Buffer, int XOffset, int YOffset,
  game_sound_output_buffer* SoundBuffer, unsigned ToneHz)
{
  GameOutputSound(SoundBuffer, ToneHz);
  RenderWeirdRectangle(Buffer, XOffset, YOffset);
}
