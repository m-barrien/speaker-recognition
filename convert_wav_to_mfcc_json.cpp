/*


*/

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
// sudo apt-get install libasound2-dev
#include <alsa/asoundlib.h>
#include <signal.h>
#include <sndfile.h>
#include "Preprocessing.h"
#include <thread>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdint>
#include "conf.h"
/*
#define N_CHANNELS 1
#define RAW_PERIOD_SAMPLE_SIZE 8192
#define SAMPLES_PER_SECOND 44100
#define N_FILTERS 40
#define N_MFCC_COEFS 40
#define FRAME_SIZE 1024
#define MAX_FREQ 22000
#define MIN_FREQ 0
#define WINDOW_OVERLAP 0.5f
#define MFCC_LOT_SIZE 5
*/

static float *float_buffer;

//seconds * SAMPLES_PER_SECOND/RAW_PERIOD_SAMPLE_SIZE
static int capturing_file_stream = true;

void exit_handler(int sig){
  if (sig == SIGINT)
  {
    /* code */
  }
  capturing_file_stream=false;
}


int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <wav_file_path> <json_file_path>" << std::endl;
    return 1;
  }

  std::string file_path = argv[1];
  std::string json_path = argv[2];

  // Signal to terminate
  signal(SIGINT, exit_handler);

  // Open the file
  SF_INFO sfinfo;
  SNDFILE *sndfile = sf_open(file_path.c_str(), SFM_READ, &sfinfo);
  if (!sndfile) {
    std::cerr << "Error opening file: " << file_path << std::endl;
    return 1;
  }
  std::cout << "Sample rate: " << sfinfo.samplerate << std::endl;
  assert(sfinfo.samplerate / 2 >= MAX_FREQ);
  std::cout << "Channels: " << sfinfo.channels << std::endl;
  assert(sfinfo.channels == N_CHANNELS);
  std::cout << "Frames or samples: " << sfinfo.frames << std::endl;
  std::cout << "Format: " << sfinfo.format << std::endl;
  assert(sfinfo.format == SF_FORMAT_WAV | SF_FORMAT_PCM_16);
  std::cout << "Duration: " << static_cast<float>(sfinfo.frames) / sfinfo.samplerate << " seconds" << std::endl;
  std::cout << "File opened successfully." << std::endl;

  // 30ms of recording 
  int minimum_samples = static_cast<int>(0.03 * sfinfo.samplerate);
  //find the next power of 2 larger than minimum_samples
  int mfcc_wav_frame_size = 1;
  while (mfcc_wav_frame_size < minimum_samples) {
    mfcc_wav_frame_size *= 2;
  }
  int mfcc_wav_batch_frame_size = mfcc_wav_frame_size * 16; // 16 batches of 30ms around 0.5segs
  // Allocate buffers
  float_buffer = new float[mfcc_wav_batch_frame_size * sfinfo.channels];

  SignalPreprocessor sProcessor = SignalPreprocessor(float_buffer, mfcc_wav_batch_frame_size, mfcc_wav_frame_size, WINDOW_OVERLAP, sfinfo.samplerate);

  int n_mel_filters = 40;
  int n_mfcc_coefs = 13;
  int min_freq = 300;
  int max_freq = 8000;
  sProcessor.buildFilterBanks(n_mel_filters, min_freq, max_freq);
  sProcessor.configureMFCC(n_mfcc_coefs); // n MFCC coefficients

  // Output MFCC buffer
  int n_mfcc_frames = sProcessor.getFrameCount();
  n_mfcc_coefs = sProcessor.getMfccCount();
  float *mfcc_buffer = new float[n_mfcc_frames * n_mfcc_coefs];

  std::cout << n_mfcc_frames  << " mfcc frames total per batch" << std::endl;
  std::cout << n_mfcc_coefs << " MFCC coefficients " << std::endl;
  std::cout << sizeof(float) * n_mfcc_frames * n_mfcc_coefs << " bytes per frame " << std::endl;

  // Create json file
  std::ofstream json_file(json_path);
  if (!json_file.is_open()) {
    std::cerr << "Error opening JSON file: " << json_path << std::endl;
    sf_close(sndfile);
    return 1;
  }
  json_file << "{" << std::endl;
  json_file << "  \"mfcc\": [" << std::endl;
  bool first_frame = true;
  while (capturing_file_stream) {
    // Read file stream and convert to MFCC
    sf_count_t read_count = sf_read_float(sndfile, float_buffer, mfcc_wav_batch_frame_size * N_CHANNELS);
    if (read_count == 0) {
      break; // End of file
    }
    if(read_count < mfcc_wav_batch_frame_size * N_CHANNELS) {
      for (int i = read_count; i < mfcc_wav_batch_frame_size * N_CHANNELS; ++i) {
        float_buffer[i] = 0.0f; // Zero padding
      }
    }
    // Convert signal to MFCC coefficients and write to MFCC buffer
    sProcessor.getMfccCoefs(&n_mfcc_frames, &n_mfcc_coefs, mfcc_buffer);
    //test that all values are not nan or inf
    bool invalid_value_found = false;
    for (int i = 0; i < n_mfcc_frames * n_mfcc_coefs; ++i) {
      if (std::isnan(mfcc_buffer[i]) || std::isinf(mfcc_buffer[i])) {
        std::cerr << "Error: Invalid value (NaN or Inf) detected in MFCC buffer at index " << i << std::endl;
        invalid_value_found = true;
        break;
      }
    }
    if (invalid_value_found) {
      continue; // Skip this batch
    }
    // Write MFCC to JSON file
    for (int i = 0; i < n_mfcc_frames; ++i) {
      if(first_frame){
        json_file <<  " [";
        first_frame = false;
      } else {
        json_file <<  ", [";
      }
      for (int j = 0; j < n_mfcc_coefs; ++j) {
        json_file << mfcc_buffer[i * n_mfcc_coefs + j];
        if (j < n_mfcc_coefs - 1) {
          json_file << ", ";
        }
      }
      json_file << "]";
      json_file << std::endl;
    }
  }

  sf_close(sndfile);
  json_file << "  ]" << std::endl;
  json_file << "}" << std::endl;
  json_file.close();
  std::cout << "MFCC conversion completed. JSON file saved at: " << json_path << std::endl;
  return 0;
}

