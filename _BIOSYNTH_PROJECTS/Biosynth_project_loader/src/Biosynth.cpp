/**
 * @file Biosynth.cpp
 * @author Erin Gee & Etienne Montenegro
 * @brief  implementation file of the Biosynth class
 * @version 1.1
 * @date 2022-04-02
 */
#include "Biosynth.h"

#include <Chrono.h>

#include "Project_list.h"
#include "audio_manager.h"
#include "biosensors.h"
#include "buttons.h"
#include "enc.h"
#include "lcd.h"
#include "led.h"

enum lcd_state
{
  BOOT = 0,
  CHANGE_SECTION,
  CURRENT_SECTION,
  START_LOGGING,
  LOGGING
};

void Biosynth::initialize()
{
  Serial.println("Erin Gee's Biosynth");

  screen::initialize();
  encoder::initialize();
  button::initialize();
  loadProject();
  audio_manager::audio_shield_initialization();
  audio_manager::mute(true);
  led::initialize();
  biosensors::initialize();

#if LOG
  session_log.initialize();
#endif

  project->setup();
  audio_manager::mute(false);
  screen::clear();
}

void Biosynth::loadProject()
{
  selected_project = selectProject(2000);

  switch (selected_project)
  { // add new projects to this switch case (just copy paste the case and change the title and class name)
  case SONG_OF_SEVEN:
    project = new SongOfSeven(&biosensors::heart, &biosensors::sc1,
                              &biosensors::resp, &biosensors::sc2);
    break;

  case WE_AS_WAVE:
    project = new WeAsWaves(&biosensors::heart, &biosensors::sc1,
                            &biosensors::resp, &biosensors::sc2);
    break;

  case RECORDER:
    project = new Recorder(&biosensors::heart, &biosensors::sc1,
                           &biosensors::resp, &biosensors::sc2);
    break;
  }

  Serial.printf("Project loaded: %s\n", project->getName());

  selectedProjectMessage(1000); // get stuck when trying to update lcd
}

void Biosynth::update()
{
  static Chrono timer;
  if (timer.hasPassed(configuration::biosensors_sample_rate_ms, true))
  {
    biosensors::update();
#if LOG
#if FOOT_PEDAL
    session_log.log_data(biosensors::heart.getRaw(), biosensors::sc1.getRaw(), biosensors::resp.getRaw(), button::foot_pedal.read());
#else
    session_log.log_data(biosensors::heart.getRaw(), biosensors::sc1.getRaw(), biosensors::resp.getRaw());
#endif
#endif
#if SEND_OVER_SERIAL
    send_over_serial(&Serial);
#endif
  }

  audio_manager::setVolume(updatePotentiometer());
  project->update();

  current_encoder_value = encoder::update(project->getNumberOfSection());
  button::update();

#if LOG
  handle_logging();
#endif

#if ADVANCE_WITH_ENCODER
  maybe_confirm_section_change();
#else
  if (button::foot_pedal.pressed() && lcd_state == CURRENT_SECTION)
  {
    advance_section();
    Serial.println("Foot pedal pressed. Advanced section");
  }
#endif

  if (lcdUpdate.hasPassed(40, true))
  {
    opening_message();
#if LOG
    
#endif

#if ADVANCE_WITH_ENCODER
    section_change();
    verify_no_touch();
#endif

    led::update(project->getLedProcessed());
  }
}


#if LOG
void Biosynth::handle_logging()
{
  
  switch (lcd_state)
  {
  case CURRENT_SECTION:
    if (button::encoder.pressed() && !session_log.is_logging())
    {
      Serial.println("Ask user to record on SD?");
      session_log.create_file();
      sprintf(screen::buffer_line_1, "Record on SD?");
      sprintf(screen::buffer_line_2, "               ");
      screen::update();
      lcd_state = START_LOGGING;
    }
    break;

  case START_LOGGING:
    if (button::encoder.pressed() && !session_log.is_logging())
    {
      Serial.println("Starting logging");
      session_log.start_logging();
      lcd_state = LOGGING;

      sprintf(screen::buffer_line_1, "  Now Logging  ");
      sprintf(screen::buffer_line_2, "              ");
      screen::update();
    }
    break;

  case LOGGING:
    if (button::encoder.pressed() && session_log.is_logging()&& !endLogging.isRunning())
    {
      Serial.println("Ending session");
      session_log.stop_logging();
      Serial.printf("Number of samples recorded: %d\n", session_log.get_num_samples());
      endLogging.restart();
      sprintf(screen::buffer_line_1, "Logging Stopped");
      sprintf(screen::buffer_line_2, "               ");
      
      screen::update();
    }

    if(endLogging.isRunning() ){
      if(endLogging.hasPassed(2000, true)){
        current_section_message();
        endLogging.stop();
      }
      
    }
  }
}

#endif

void Biosynth::opening_message()
{
  static bool do_once{false};

  if (!do_once)
  {
    sprintf(screen::buffer_line_1, "Hello!");
    sprintf(screen::buffer_line_2, "I am board #%d", configuration::board_id + 1);
    screen::update();
    do_once = true;
  }
  else if (openingtimer.hasPassed(configuration::opening_message_time) && do_once)
  {
    current_section_message();
    openingtimer.restart();
    openingtimer.stop();
  }
}

void Biosynth::maybe_confirm_section_change()
{
  if (button::encoder.pressed() && lcd_state == CHANGE_SECTION)
  {
    Serial.println("Section change confirmed");
    last_section = current_section;
    current_section = current_encoder_value;
    project->changeSection(current_encoder_value);
    current_section_message();
    confirmTimer.restart();
    confirmTimer.stop();
  }
}

void Biosynth::verify_no_touch()
{
  if (confirmTimer.hasPassed(configuration::confirmation_delay) &&
      lcd_state == CHANGE_SECTION)
  {
    encoder::set_value(current_section);
    current_section_message();
    confirmTimer.restart();
    confirmTimer.stop();
  }
}

void Biosynth::section_change()
{
  if (current_encoder_value != current_section)
  {
    section_confirm_message(current_encoder_value);
    if (!confirmTimer.isRunning())
    {
      confirmTimer.start();
    }
  }
}

void Biosynth::section_confirm_message(const int encoder_value)
{
  sprintf(screen::buffer_line_1, "%s", project->getSectionTitle(current_encoder_value));

  sprintf(screen::buffer_line_2, "   Confirm ?   ");
  lcd_state = CHANGE_SECTION;
  screen::update();
}

void Biosynth::current_section_message()
{
  sprintf(screen::buffer_line_1, "%s", project->getSectionTitle(current_section));
  sprintf(screen::buffer_line_2, "   BIOSYNTH %d ", configuration::board_id + 1);
  lcd_state = CURRENT_SECTION;
  screen::update();
}

void Biosynth::advance_section()
{
  current_section++;
  current_section = current_section % project->getNumberOfSection();
  project->changeSection(current_section);
  current_section_message();
}

float Biosynth::updatePotentiometer()
{
  float knob2 = analogRead(pins::audio_shield::volume);
  if (knob2 != vol)
  {
    vol = (knob2 / 1023) * 0.8;
  }
  return vol;
}

ProjectList Biosynth::selectProject(
    const int &timeout)
{ // need to be modified if more than two project
  Chrono waitTime;
  waitTime.restart();
  int project = 0;
  while (!waitTime.hasPassed(timeout))
  {
    project = button::getEncoder();
  }

  if (project == 1)
  { // project selected when button not pressed on boot

    return RECORDER; // project 1 is recorder, change for WeAsWaves if used for performance
  }
  else
  { // project selected when button not pressed on boot
    return SONG_OF_SEVEN;
  }
}

void Biosynth::selectedProjectMessage(const int &displayTime)
{
  Chrono waitTime;

  sprintf(screen::buffer_line_1, "    Biosynth   ");
  sprintf(screen::buffer_line_2, project->getName());
  screen::update();
  waitTime.restart();
  while (!waitTime.hasPassed(displayTime))
  {
    // wait in function
  }
}

void Biosynth::send_over_serial(Print *output)
{

  output->printf("%d,%.2f,%.2f,%.2f\n", configuration::board_id,
                 biosensors::heart.getNormalized(),
                 biosensors::sc2.getSCR(),
                 biosensors::resp.getNormalized());
}

#if PLOT_SENSOR
void Biosynth::plot_sampled_data()
{
  Serial.printf("%.2f,%.2f,%.2f,%.2f", biosensors::heart.getNormalized(), biosensors::sc1.getSCR(), biosensors::sc2.getSCR(), biosensors::resp.getNormalized());
  Serial.println();
}
#endif
