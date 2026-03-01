#pragma once

#include "kernel/card_model.h"

bool isMissionRunning(logicCardType type, cardState state);
bool isMissionFinished(logicCardType type, cardState state);
bool isMissionStopped(logicCardType type, cardState state);

