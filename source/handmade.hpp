#ifndef HANDMADE_HPP
#define HANDMADE_HPP

#ifndef HANDMDE_GLOBAL_DEFINE
#define HANDMDE_GLOBAL_DEFINE

#define internal_func static
#define local_persist static
#define global_variable static

#endif 

#include <cstdint>
#include <cstddef>



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



struct game_state
{
  bool IsInitialized;
  int XOffset = 0;
  int YOffset = 0;
  unsigned ToneHz = 260;
  float Angle = 0.0f;
  unsigned Volume = 4000;
};



#ifdef HANDMADE_INTERNAL
struct debug_file_result
{
  void* Memory;
  uint64_t Size;
};

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) bool name(char const* FileName, debug_file_result* Result)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);


#define DEBUG_PLATFORM_WRITE_ENTINE_FILE(name) bool name(char const* FileName, void const* Buffer, uint64_t Size) 
typedef DEBUG_PLATFORM_WRITE_ENTINE_FILE(debug_platform_write_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(debug_file_result* File);
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#endif

struct game_memory
{
  void* PermanentStorage = NULL;
  uint64_t PermanentStorageSize = 0;

  void* TransientStorage = NULL;
  uint64_t TransientStorageSize = 0;

#ifdef HANDMADE_INTERNAL
  debug_platform_read_entire_file* DEBUGPlatformReadEntireFile = NULL;
  debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile = NULL;
  debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory = NULL;
#endif
};


#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* Memory, \
                                               game_offscreen_buffer* Buffer, \
                                               game_inputs* Inputs)

typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{
    (void) Memory;
    (void) Buffer;
    (void) Inputs;
}


#define GAME_GET_SOUND_SAMPLE(name) void name(game_memory* Memory, \
                                              game_sound_output_buffer* SoundBuffer)

typedef GAME_GET_SOUND_SAMPLE(game_get_sound_sample);

GAME_GET_SOUND_SAMPLE(GameGetSoundSampleStub)
{
    (void) Memory;
    (void) SoundBuffer;
}

#endif
