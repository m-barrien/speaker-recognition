/*


*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
// sudo apt-get install libasound2-dev
#include <alsa/asoundlib.h>
#include <signal.h>
#include <thread>
#include <string>
#include <mutex>
#include <iostream>
#include <cstdint>
#include "conf.h"

static std::mutex raw_buffer_mutex;
static std::string dev_name = "hw:1,0";
static char *buffer;
static int16_t *int_buffer;
static int frame_overflow=0;
static float *float_buffer;

//seconds * SAMPLES_PER_SECOND/RAW_PERIOD_SAMPLE_SIZE
static int capturing = 1 * SAMPLES_PER_SECOND/RAW_PERIOD_SAMPLE_SIZE;

void exit_handler(int sig){
  if (sig == SIGINT)
  {
    /* code */
  }
  capturing=0;
}

void *capture_mic() {
  //long loops;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  


  /* Open PCM device for recording (capture). */
  rc = snd_pcm_open(&handle, dev_name.c_str(),
                    SND_PCM_STREAM_CAPTURE, 0);
  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    exit(1);
  }
  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);
  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);
  /* Set the desired hardware parameters. */
  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params,
                      SND_PCM_ACCESS_RW_INTERLEAVED);
  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params,
                              SND_PCM_FORMAT_S16_LE);
  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, N_CHANNELS);
  /* 44100 bits/second sampling rate (CD quality) */
  val = SAMPLES_PER_SECOND;
  snd_pcm_hw_params_set_rate_near(handle, params,
                                  &val, &dir);
  /* Set period size to 32 frames. */
  frames = RAW_PERIOD_SAMPLE_SIZE;
  snd_pcm_hw_params_set_period_size_near(handle,
                              params, &frames, &dir);
  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    exit(1);
  }
  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params,
                                      &frames, &dir);
  size = frames * 2 * N_CHANNELS; 
  /* 2 bytes/sample, 2 channels */
  
  /* We want to loop for 5 seconds */
  snd_pcm_hw_params_get_period_time(params,
                                         &val, &dir);
  //loops = 50000 / val;
  while (capturing >0) {
    //loops--;
    raw_buffer_mutex.lock();
    rc = snd_pcm_readi(handle, buffer, frames);
    if (rc == -EPIPE) {
      /* EPIPE means overrun */
      fprintf(stderr, "overrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr,
              "error from read: %s\n",
              snd_strerror(rc));
    } else if (rc != (int)frames) {
      fprintf(stderr, "short read, read %d frames\n", rc);
    } 
    else {
      frame_overflow++;
      raw_buffer_mutex.unlock();
    }
    rc = write(1, buffer, size);
    if (rc != size)
      fprintf(stderr,
              "short write: wrote %d bytes\n", rc);
  }
  snd_pcm_drain(handle);
  snd_pcm_close(handle);


  pthread_exit(NULL);
}

int main() {
  //Señal para terminar
  signal (SIGINT,exit_handler);

  buffer = new char[RAW_PERIOD_SAMPLE_SIZE*2*N_CHANNELS];
  int_buffer = new int16_t[RAW_PERIOD_SAMPLE_SIZE*N_CHANNELS];
  float_buffer = new float[RAW_PERIOD_SAMPLE_SIZE*N_CHANNELS];

  std::thread capture_thread(capture_mic);
  capture_thread.join();
  //std::cout << "overflow " << frame_overflow << std::endl;
  return 0;
}

