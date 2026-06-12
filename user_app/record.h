#ifndef RECORD_H
#define RECORD_H

#define QUEUE_SIZE 8 //  Cuántos frames podemos almacenar en RAM antes de bloquear

int init_video_capture(int start_width, int start_height);

void record_frame();

void clean_video_capture();

#endif
