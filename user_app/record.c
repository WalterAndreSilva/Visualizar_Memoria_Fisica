#define _POSIX_C_SOURCE 200809L

#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <pthread.h>
#include "../share.h"

#include "record.h"


int record_w = 0;
int record_h = 0;
int data_size = 0;
FILE *ffmpeg = NULL;
GLuint pbo[2] = {0, 0};
int pbo_index = 0;
int pbo_next_index = 1;
int first_frame_pbo = 1;


typedef struct {
    uint8_t* buffers[QUEUE_SIZE];
    int head;
    int tail;
    int count;
    int frame_size;
    int stop_flag;

    FILE* ffmpeg_pipe;

    pthread_mutex_t mutex;
    pthread_cond_t cond_not_empty;
    pthread_cond_t cond_not_full;
} FrameQueue;


FrameQueue video_queue;
pthread_t writer_thread;

void* ffmpeg_writer_worker(void* arg)
{
    FrameQueue* q = (FrameQueue*)arg;

    while (1) {
        pthread_mutex_lock(&q->mutex);

        while (q->count == 0 && !q->stop_flag) {
            pthread_cond_wait(&q->cond_not_empty, &q->mutex);
        }

        if (q->count == 0 && q->stop_flag) {
            pthread_mutex_unlock(&q->mutex);
            break;
        }

        uint8_t* frame_data = q->buffers[q->tail];
        q->tail = (q->tail + 1) % QUEUE_SIZE;
        q->count--;

        pthread_cond_signal(&q->cond_not_full);
        pthread_mutex_unlock(&q->mutex);

        if (q->ffmpeg_pipe) {
            fwrite(frame_data, 1, q->frame_size, q->ffmpeg_pipe);
        }
    }
    return NULL;
}


int init_video_capture(int start_width, int start_height)
{
    record_w = start_width & ~1;
    record_h = start_height & ~1;
    data_size = record_w * record_h * 3;

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    char filename[128];
    strftime(filename, sizeof(filename), "RAM_map_%Y-%m-%d_%H-%M-%S.mp4", tm_info);
    printf("Starting recording in: %s\n", filename);

    glGenBuffers(2, pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[0]);
    glBufferData(GL_PIXEL_PACK_BUFFER, data_size, 0, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[1]);
    glBufferData(GL_PIXEL_PACK_BUFFER, data_size, 0, GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    char ffmpeg_cmd[512];
    snprintf(ffmpeg_cmd, sizeof(ffmpeg_cmd),
             "ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgb24 "
             "-s %dx%d -r %d -i - -vf vflip -c:v libx264 -preset ultrafast -crf 23 -pix_fmt yuv420p %s",
             record_w, record_h, TARGET_FPS, filename);

    ffmpeg = popen(ffmpeg_cmd, "w");
    if (!ffmpeg) {
        perror("Error opening pipe to FFmpeg");
        glDeleteBuffers(2, pbo);
        return -1;
    }
    video_queue.head = 0;
    video_queue.tail = 0;
    video_queue.count = 0;
    video_queue.frame_size = data_size;
    video_queue.stop_flag = 0;
    video_queue.ffmpeg_pipe = ffmpeg;

    pthread_mutex_init(&video_queue.mutex, NULL);
    pthread_cond_init(&video_queue.cond_not_empty, NULL);
    pthread_cond_init(&video_queue.cond_not_full, NULL);

    for (int i = 0; i < QUEUE_SIZE; ++i) {
        video_queue.buffers[i] = (uint8_t*)malloc(data_size);
    }
    if (pthread_create(&writer_thread, NULL, ffmpeg_writer_worker, &video_queue) != 0) {
        perror("Error creating video thread");
        for (int i = 0; i < QUEUE_SIZE; ++i) {
            free(video_queue.buffers[i]);
        }
        pthread_mutex_destroy(&video_queue.mutex);
        pthread_cond_destroy(&video_queue.cond_not_empty);
        pthread_cond_destroy(&video_queue.cond_not_full);
        pclose(ffmpeg);
        ffmpeg = NULL;
        glDeleteBuffers(2, pbo);
        return -1;
    }
    return 0;
}

void record_frame()
{
    if (ffmpeg) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[pbo_index]);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, record_w, record_h, GL_RGB, GL_UNSIGNED_BYTE, 0);
        if (!first_frame_pbo) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo[pbo_next_index]);
            GLubyte* src = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

            if (src) {
                pthread_mutex_lock(&video_queue.mutex);

                if (video_queue.count == QUEUE_SIZE) {
                    printf("FFmpeg is slow\n");
                    // Bloquear el hilo principal para grabar el 100% de los frames
                    while (video_queue.count == QUEUE_SIZE) {
                        pthread_cond_wait(&video_queue.cond_not_full, &video_queue.mutex);
                    }
                }
                memcpy(video_queue.buffers[video_queue.head], src, data_size);
                video_queue.head = (video_queue.head + 1) % QUEUE_SIZE;
                video_queue.count++;

                pthread_cond_signal(&video_queue.cond_not_empty);

                pthread_mutex_unlock(&video_queue.mutex);
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            }
        } else {
            first_frame_pbo = 0;
        }

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        pbo_next_index = pbo_index;
        pbo_index = (pbo_index + 1) % 2;
    }
}

void clean_video_capture()
{
    if (ffmpeg) {
        pthread_mutex_lock(&video_queue.mutex);
        video_queue.stop_flag = 1;
        pthread_cond_signal(&video_queue.cond_not_empty);
        pthread_mutex_unlock(&video_queue.mutex);
        pthread_join(writer_thread, NULL);
        for (int i = 0; i < QUEUE_SIZE; ++i) {
            free(video_queue.buffers[i]);
        }
        pthread_mutex_destroy(&video_queue.mutex);
        pthread_cond_destroy(&video_queue.cond_not_empty);
        pthread_cond_destroy(&video_queue.cond_not_full);

        pclose(ffmpeg);
    }
    glDeleteBuffers(2, pbo);
}
