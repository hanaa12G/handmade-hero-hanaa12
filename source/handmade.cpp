#include "handmade.hpp"
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

internal_func uint32_t
SafeTruncateNumber(uint64_t Input)
{
  HANDMADE_ASSERT(Input <= 0xffffffff);
  return (uint32_t) Input;
}



internal_func void
GameOutputSound(game_sound_output_buffer* SoundOutputBuffer, unsigned ToneHz)
{
  local_persist float Angle = 0.0f;
  local_persist unsigned Volume = 4000;

  unsigned SampleCountToOutput = SoundOutputBuffer->SampleCountToOutput;
  int16_t* Samples = SoundOutputBuffer->Samples;
  float    WavePeriod = (float) SoundOutputBuffer->SamplesPerSecond / (float) ToneHz;



  float AngleStep = (2 * PI / (float) WavePeriod);

  for (unsigned SampleIndex = 0;
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

void GameUpdateAndRender(game_offscreen_buffer* Buffer,
  game_sound_output_buffer* SoundBuffer,
  game_inputs* Inputs,
  game_memory* Memory)
{
  HANDMADE_ASSERT(Memory->PermanentStorageSize >= sizeof(game_state));

  game_state* GameState = (game_state*) (Memory->PermanentStorage);
  if (!GameState->IsInitialized)
  {
    GameState->IsInitialized = true;
    GameState->XOffset = 0;
    GameState->YOffset = 0;
    GameState->XSpeed  = 0;
    GameState->YSpeed  = 1;
    GameState->ToneHz = 260;
    GameState->Acceleration = 1;

    debug_file_result Result;
    if (PlatformReadEntireFile(__FILE__, &Result))
    {
      char const* TempOutFile = "W:\\handmade-hero\\temp.out";
      PlatformWriteEntireFile(TempOutFile, Result.Memory, Result.Size);
      PlatformFreeFileMemory(&Result);
    }
  }

  game_input_controller* Input = &Inputs->Controllers[0];

  if (Input->IsAnalog)
  {
  }
  else
  {
    game_button_state LeftButton = Input->LeftButton;
    game_button_state RightButton = Input->RightButton;
    game_button_state UpButton = Input->UpButton;
    game_button_state DownButton = Input->DownButton;
    if (LeftButton.HalfTransitionCount > 0 && LeftButton.IsButtonEndedDown)
    {
      GameState->XSpeed += GameState->Acceleration;
    }
    if (RightButton.HalfTransitionCount > 0 && RightButton.IsButtonEndedDown)
    {
      GameState->XSpeed -= GameState->Acceleration;
    }

    if (UpButton.HalfTransitionCount > 0 && UpButton.IsButtonEndedDown)
    {
      GameState->YOffset -= GameState->YSpeed;
      GameState->ToneHz -= 10;
    }

    if (DownButton.HalfTransitionCount > 0 && DownButton.IsButtonEndedDown)
    {
      GameState->YOffset += GameState->YSpeed;
      GameState->ToneHz += 10;
    }
    
  }

  GameState->XOffset += GameState->XSpeed;
  GameState->YOffset += GameState->YSpeed;
  GameOutputSound(SoundBuffer, GameState->ToneHz);
  RenderWeirdRectangle(Buffer, GameState->XOffset, GameState->YOffset);
}

