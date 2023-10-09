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

struct game_input_controllers
{
  game_input_controller Controllers[4];
};


void GameUpdateAndRender(game_offscreen_buffer* Buffer,
                         game_sound_output_buffer* SoundBuffer,
                         game_input_controllers* Inputs);

#endif
