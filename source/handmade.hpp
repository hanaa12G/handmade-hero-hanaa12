#ifndef HANDMADE_HPP
#define HANDMADE_HPP


struct game_offscreen_buffer 
{
  void*      Data;
  int        Width;
  int        Height;
  int        Pitch;
};


void GameUpdateAndRender(game_offscreen_buffer* Buffer, int XOffset, int YOffset);

#endif
