#pragma once

#include "kernel/card_model.h"

bool parseV3CardTypeToken(const char* cardType, logicCardType& outType);
bool isV3FieldAllowedForSourceType(logicCardType sourceType, const char* field);
bool isV3OperatorAllowedForField(const char* field, const char* op);

