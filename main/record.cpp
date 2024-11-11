#include "record.h"
#include "buffer.hpp"
#include "writer.hpp"

extern Buffer buffmpu;

void record_proc(void *p)
{
  BuffHeadRead *hr = buffmpu.r_heads.new_head();
  Writer writer(hr, "/sdcard/%#.dat", 32, 1024*10);
  hr->set_to_newest();
  writer.run();
}
