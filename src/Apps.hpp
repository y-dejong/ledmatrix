#pragma once

#include <unordered_map>
#include <string>

#include "ControlServer.hpp"

//#include "apps/animation/Animation.hpp"
#include "apps/clock/Clock.hpp"
#include "apps/pictureframe/PictureFrame.hpp"
#include "apps/effects/effects.hpp"
#include "apps/conway/conway.hpp"

static const std::unordered_map<std::string, AppFunction> APPS{
  {"clock", Clock::runTask},
  {"pictureframe", PictureFrame::run_task},
  {"rain", rain_task},
  {"fire", fire_task},
  {"conway", conway_task}
  //{"animate", runAnimationTask}
};
