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

struct game_button_state
{
  bool IsButtonEndedDown;
  int  HalfTransitionCount;
};


struct game_input_controller
{
  bool IsAnalog;

  float StartX;
  float EndX;
  float MaxX;

  float StartY;
  float EndY;
  float MaxY;

  union
  {
    game_button_state Buttons[6];

    struct 
    {
      game_button_state LeftButton;
      game_button_state RightButton;
      game_button_state UpButton;
      game_button_state DownButton;

      game_button_state LeftShoulderButton;
      game_button_state RightShoulderButton;
    };
  };
};


struct game_clocks
{
  long long EslapsedTime;
};


struct game_inputs
{
  game_clocks Timers;
  game_input_controller Controllers[4];
};


struct game_memory
{
  void* PermanentStorage;
  uint64_t PermanentStorageSize;

  void* TransientStorage;
  uint64_t TransientStorageSize;
};

struct game_state
{
  bool IsInitialized;
  int XOffset = 0;
  int YOffset = 0;
  int XSpeed  = 0;
  int YSpeed  = 1;
  unsigned ToneHz = 260;
  int Acceleration = 1;
};



void GameUpdateAndRender(game_offscreen_buffer* Buffer,
                         game_sound_output_buffer* SoundBuffer,
                         game_inputs* Inputs,
                         game_memory* Memory);

#endif
