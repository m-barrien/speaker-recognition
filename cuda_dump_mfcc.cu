/*
FUNCION PARA CONVERTIR UN BUFFER DE SEÑAL A COEFICIENTES DE ESPECTRO MEL MFCC

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "CUDAPreprocessing.h"
#include <thread>
#include <string>
#include <mutex>
#include <iostream>
#include <cstdint>
#include "conf.h"

static float *float_buffer;

//seconds * SAMPLES_PER_SECOND/RAW_PERIOD_SAMPLE_SIZE
static int frames_to_process = 10;
static int frames_to_process_i = frames_to_process;

void exit_handler(int sig){
  if (sig == SIGINT)
  {
    /* code */
  }
  frames_to_process=0;
}

int main() {
  //Señal para terminar
  signal (SIGINT,exit_handler);

  float_buffer = new float[RAW_PERIOD_SAMPLE_SIZE*N_CHANNELS];

  CUDASignalPreprocessor sProcessor =  CUDASignalPreprocessor(float_buffer, RAW_PERIOD_SAMPLE_SIZE, FRAME_SIZE, WINDOW_OVERLAP, SAMPLES_PER_SECOND);

  sProcessor.buildFilterBanks(N_FILTERS,MIN_FREQ,MAX_FREQ); 
  sProcessor.configureMFCC(N_MFCC_COEFS); //n mfcc coefs

  //output mfcc buffer
  int n_mfcc_frames =sProcessor.getFrameCount();
  int n_mfcc_coefs =sProcessor.getMfccCount();
  float *mfcc_buffer = new float[n_mfcc_frames * n_mfcc_coefs];

  int time_s = clock();
  while(frames_to_process >= 0){

    for (int i = 0; i < RAW_PERIOD_SAMPLE_SIZE*N_CHANNELS; ++i)
    {
      float_buffer[i] = ((float)rand()/(float)(RAND_MAX)) * 2 - 1;; //randomsignal
    }
    //convert signal to mfcc coefs and write to mfcc_buffer
    sProcessor.getMfccCoefs(&n_mfcc_frames,&n_mfcc_coefs,mfcc_buffer);
    
    //write(1, mfcc_buffer, sizeof(float)*n_mfcc_frames*n_mfcc_coefs);
    frames_to_process--;
  }
  double runtime = (double)(clock() - time_s) / (double)CLOCKS_PER_SEC;
  std::cout << runtime << " segundos para procesar " << frames_to_process_i << " frames de audio o " << frames_to_process_i * RAW_PERIOD_SAMPLE_SIZE/SAMPLES_PER_SECOND << " segundos de audio real" <<std::endl;
  return 0;
}

