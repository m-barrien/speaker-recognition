/*


*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
// sudo apt-get install libasound2-dev
#include <alsa/asoundlib.h>
#include <signal.h>
#include "Preprocessing.h"
#include "udpsender.h"
#include <thread>
#include <string>
#include <mutex>
#include <iostream>
#include <cstdint>
#include "conf.h"

static std::mutex raw_buffer_mutex;
static std::string dev_name = "hw:0,0";
static char *buffer;
static int16_t *int_buffer;
static int frame_overflow=0;
static float *float_buffer;

//seconds * SAMPLES_PER_SECOND/RAW_PERIOD_SAMPLE_SIZE
static int capturing = true;

void exit_handler(int sig){
  if (sig == SIGINT)
  {
    /* code */
  }
  capturing=false;
}

void *capture_mic() {
  //long loops;
  int rc;
  //int size;
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
  //size = frames * 2 * N_CHANNELS; 
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
    usleep(100);
    /*
    rc = write(1, buffer, size);
    if (rc != size)
      fprintf(stderr,
              "short write: wrote %d bytes\n", rc);
    */
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

  SignalPreprocessor sProcessor =  SignalPreprocessor(float_buffer, RAW_PERIOD_SAMPLE_SIZE, FRAME_SIZE, WINDOW_OVERLAP, SAMPLES_PER_SECOND);
  std::thread capture_thread(capture_mic);

  sProcessor.buildFilterBanks(N_FILTERS,MIN_FREQ,MAX_FREQ); 
  sProcessor.configureMFCC(N_MFCC_COEFS); //20 mfcc coefs

  //output mfcc buffer
  int n_mfcc_frames =sProcessor.getFrameCount();
  int n_mfcc_coefs =sProcessor.getMfccCount();
  float *mfcc_buffer = new float[n_mfcc_frames * n_mfcc_coefs];

  //udp output buffer
  char *udp_mfcc_buffer = new char[sizeof(float) * n_mfcc_frames * n_mfcc_coefs * MFCC_LOT_SIZE];
  //build udp sender
  std::string addr = UDP_ADDR;
  int port = UDP_PORT;
  udp_client_server::udp_client udpsender = udp_client_server::udp_client(addr.c_str(), port);
  
  //write(1, &n_mfcc_coefs, sizeof(int));
  std::cout << "Sending to " << addr << ":" << port <<std::endl;
  std::cout << n_mfcc_frames * MFCC_LOT_SIZE << " frames total" << std::endl;
  std::cout << n_mfcc_coefs << " MFCC coefficients " << std::endl;
  std::cout << sizeof(float)*n_mfcc_frames*n_mfcc_coefs  << " bytes p frame " << std::endl;
  std::cout << sizeof(float)*n_mfcc_frames*n_mfcc_coefs * MFCC_LOT_SIZE << " total bytes " << std::endl;

  size_t cycles =0;
  int bytes_per_mfcc_bunch = sizeof(float)*n_mfcc_frames*n_mfcc_coefs;
  while(capturing){
    if (frame_overflow < 1) {
      //sleep 5ms waiting for worload
      usleep(5000);
      continue;
    };
    raw_buffer_mutex.lock();
    memcpy(int_buffer, buffer, RAW_PERIOD_SAMPLE_SIZE*2*N_CHANNELS);
    frame_overflow--;
    raw_buffer_mutex.unlock();
    for (int i = 0; i < RAW_PERIOD_SAMPLE_SIZE*N_CHANNELS; ++i)
    {
      float_buffer[i] = (float) int_buffer[i]/32768.f;
    }
    //convert signal to mfcc coefs and write to mfcc_buffer
    sProcessor.getMfccCoefs(&n_mfcc_frames,&n_mfcc_coefs,mfcc_buffer);
    
    //write(1, mfcc_buffer, sizeof(float)*n_mfcc_frames*n_mfcc_coefs);
    
    memcpy((udp_mfcc_buffer + (bytes_per_mfcc_bunch*(cycles % MFCC_LOT_SIZE)) ), mfcc_buffer, bytes_per_mfcc_bunch);
    if (cycles % MFCC_LOT_SIZE == MFCC_LOT_SIZE -1)
    {
      udpsender.send((char*)udp_mfcc_buffer,bytes_per_mfcc_bunch*MFCC_LOT_SIZE);  
    }
    cycles++;

  }
  
  capture_thread.join();
  //std::cout << "overflow " << frame_overflow << std::endl;
  return 0;
}

