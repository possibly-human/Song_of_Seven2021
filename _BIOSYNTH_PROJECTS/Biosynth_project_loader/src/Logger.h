/**
 * @file Logger.h
 * @author Etienne Montengro
 * @brief Logger class that manages files creation and writting to it
 *        Modify the log_data method to accept as argument what you want to log and its formating.
 * @version 1.1
 * @date 2022-04-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include <SD.h>
#include <RingBuf.h>

#include "biosensors.h"

class FsFile;


//add name checking not to overwrite older log in sd card
class logger{
  FsFile recording;

  const char* extension {"txt"};
  const char* filename {"session_recording"};

  const int LOG_INTERVAL_USEC{10000}; //unused variable

  const int file_size{8}; // in megabyte
  const int LOG_FILE_SIZE{file_size * 1024 * 1024};
  bool logging{false};

  int numSamples{0};

 

  public:
  /**
   * @brief intilialize the sd card, blocks into an infinite while if it can't
   */
  void initialize();

  /**
   * @brief Create a file on the sd card
   */
  void create_file();

  /**
   * @brief write the data to the file.
   * @param signals sample of data to log
   */
  void log_data(const int heart, const int gsr, const int resp); 

  void log_data(const int heart, const int gsr, const int resp, const bool feelingIt); 

  /**
   * @brief starts datalogging
   */
  void start_logging();

  /**
   * @brief stops data logging
   */
  void stop_logging();

  /**
   * @brief return true if device is currently logging data
   */
  bool is_logging();

  inline int get_num_samples(){
    return numSamples;
    };
};