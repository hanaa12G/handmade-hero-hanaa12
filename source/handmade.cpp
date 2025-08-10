#include "handmade.hpp"
#include <cmath>
#include <cstdint>

#define PI 3.14159265359f
#ifdef HANDMADE_SLOW 
#define HANDMADE_ASSERT(condition) \
  if (!(condition)) \
  { \
    char* s = NULL; \
    *s = 0; \
  }
#else
#define HANDMADE_ASSERT(condition)
#endif


#define ARRAY_SIZE(x) sizeof((x)) / sizeof((x)[0])

internal_func uint32_t
SafeTruncateNumber(uint64_t Input)
{
  HANDMADE_ASSERT(Input <= 0xffffffff);
  return (uint32_t) Input;
}



internal_func void
GameOutputSound(game_sound_output_buffer* SoundOutputBuffer, game_state* GameState)
{
  unsigned ToneHz = GameState->ToneHz;
  float* Angle = &GameState->Angle;
  unsigned Volume = GameState->Volume;

  unsigned SampleCountToOutput = SoundOutputBuffer->SampleCountToOutput;
  int16_t* Samples = SoundOutputBuffer->Samples;
  float    WavePeriod = (float) SoundOutputBuffer->SamplesPerSecond / (float) ToneHz;

  float AngleStep = (2 * PI / (float) WavePeriod);

  for (unsigned SampleIndex = 0;
      SampleIndex < SampleCountToOutput;
      ++SampleIndex)
  {
    float t = *Angle; 

    int16_t SampleValue = (int16_t) (sinf(t) * Volume);

    *Samples++ = SampleValue;
    *Samples++ = SampleValue;

    *Angle += AngleStep;
    if (*Angle > 2 * PI) {
      *Angle -= 2 * PI;
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

      *Pixel++ = (uint32_t)((255 << 24) | (Green << 8) | (Blue << 16));

    }

    // Rows += BytesPerPixel * Buffer->Width;
    Rows += Buffer->Pitch;
  }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  HANDMADE_ASSERT(Memory->PermanentStorageSize >= sizeof(game_state));

  game_state* GameState = (game_state*) (Memory->PermanentStorage);
  if (!GameState->IsInitialized)
  {
    GameState->IsInitialized = true;
    GameState->XOffset = 0;
    GameState->YOffset = 0;
    GameState->ToneHz = 260;
    GameState->Angle = 0.0f;
    GameState->Volume = 4000;

    debug_file_result Result;
    if (Memory->DEBUGPlatformReadEntireFile(__FILE__, &Result))
    {
      char const* TempOutFile = "W:\\handmade-hero\\temp.out";
      Memory->DEBUGPlatformWriteEntireFile(TempOutFile, Result.Memory, Result.Size);
      Memory->DEBUGPlatformFreeFileMemory(&Result);
    }
  }


  for (unsigned ControllerIndex = 0;
       ControllerIndex < ARRAY_SIZE(Inputs->Controllers);
       ++ControllerIndex)
  {
    game_input_controller* Controller = &Inputs->Controllers[ControllerIndex];

    if (Controller->IsAnalog)
    {
      GameState->XOffset += (int) (4.0f * Controller->StickAverageX);
      GameState->YOffset += (int) (4.0f * Controller->StickAverageY);
      GameState->ToneHz  = 256 + (int) (128.0f * Controller->StickAverageY);
    }
    else
    {
      if (Controller->MoveLeft.EndedDown)
      {
        GameState->XOffset += 10;
      }
      if (Controller->MoveRight.EndedDown)
      {
        GameState->XOffset -= 10;
      }
      if (Controller->MoveUp.EndedDown)
      {
        GameState->YOffset += 10;
        GameState->ToneHz += 5;
      }
      if (Controller->MoveDown.EndedDown)
      {
        GameState->YOffset -= 10;
        GameState->ToneHz -= 5;
      }
    }
  }

  if (GameState->ToneHz < 65) {
    GameState->ToneHz = 65;
  } else if (GameState->ToneHz >= 523) {
    GameState->ToneHz = 523;
  }

  RenderWeirdRectangle(Buffer, GameState->XOffset, GameState->YOffset);
}

extern "C" GAME_GET_SOUND_SAMPLE(GameGetSoundSample)
{
      game_state* GameState = (game_state*) (Memory->PermanentStorage);
      GameOutputSound(SoundBuffer, GameState);
}
