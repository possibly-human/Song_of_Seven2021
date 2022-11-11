/**
 * @file buttons.h
 * @author Etienne Montenegro
 * @brief Module interfacing with the different buttons in the project
 * @version 1.1
 * @date 2022-04-02
 */
#pragma once
#include <Bounce2.h>
#include "pins.h"

namespace button{

    extern Button encoder;
    extern Button foot_pedal;
    const int refresh_rate{1};

    /** @brief Initialization routine of the component
     */
    void initialize();

    /** @brief Update the button state
     */
    void update();

    bool getEncoder();

}//namespace button