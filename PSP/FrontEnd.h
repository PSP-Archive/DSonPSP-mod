#ifndef FRONTEND
#define FRONTEND

  u16 arm9_gdb_port;
  u16 arm7_gdb_port;

  int enable_sound;
  int disable_limiter;
  int cpu_ratio;
  int lang;
  int swap;
  int showfps;
  int vertical;
  int frameskip;
  char *nds_file;
  char *cflash_disk_image_file;



void DoConfig();

void DSEmuGui(char *out);

#endif
