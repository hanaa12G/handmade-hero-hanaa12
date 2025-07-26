#ifndef HANDMADE_HPP
#define HANDMADE_HPP

#ifndef HANDMDE_GLOBAL_DEFINE
#define HANDMDE_GLOBAL_DEFINE

#define internal_func static
#define local_persist static
#define global_variable static

#endif 



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
  bool EndedDown;
  int  HalfTransitionCount;
};


struct game_input_controller
{
  bool IsAnalog;
  float StickAverageX;
  float StickAverageY;

  union
  {
    game_button_state Buttons[12];

    struct 
    {
      game_button_state MoveLeft;
      game_button_state MoveRight;
      game_button_state MoveUp;
      game_button_state MoveDown;

      game_button_state ActionLeft;
      game_button_state ActionRight;
      game_button_state ActionUp;
      game_button_state ActionDown;

      game_button_state LeftShoulderButton;
      game_button_state RightShoulderButton;

      game_button_state StartButton;
      game_button_state BackButton;
    };
  };
};


struct game_clocks
{
  float EslapsedTime;
};


#define GAMEINPUT_KEYBOARD_INDEX 0
struct game_inputs
{
  game_clocks Timers;
  game_input_controller Controllers[5];
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
  unsigned ToneHz = 260;
};



#ifdef HANDMADE_INTERNAL
struct debug_file_result
{
  void* Memory;
  uint64_t Size;
};

internal_func bool PlatformReadEntireFile(char const* FileName, debug_file_result* Result);
internal_func bool PlatformWriteEntireFile(char const* FileName, void const* Buffer, uint64_t Size); 
internal_func void PlatformFreeFileMemory(debug_file_result* File);

#endif


internal_func
void GameUpdateAndRender(game_memory* Memory,
        game_offscreen_buffer* BackBuffer,
        game_inputs* Inputs);

internal_func
void GameGetSoundSample(game_memory* Memory,
        game_sound_output_buffer* SoundBuffer);

#endif
