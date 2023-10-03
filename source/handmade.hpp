#ifndef HANDMADE_HPP
#define HANDMADE_HPP

struct game_offscreen_buffer 
{
  void*      Data;
  int        Width;
  int        Height;
  int        Pitch;
};

struct game_sound_output_buffer
{
  unsigned SamplesPerSecond;
  unsigned SampleCountToOutput;
  int16_t* Samples;
};


void GameUpdateAndRender(game_offscreen_buffer* Buffer, int XOffset, int YOffset,
                         game_sound_output_buffer* SoundBuffer);

#endif
