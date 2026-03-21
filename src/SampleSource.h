// src/SampleSource.h
//----------------------------------
// Pulse Deck TS3 Source Code
// Copyright (c) 2015 Davide (Irkanot)
// All rights reserved
// Contact: support@pulse-deck-ts3.local
//----------------------------------

#pragma once

#include "SampleProducer.h"

// Interface SampleSource
class SampleSource
{
  public:
	virtual int readSamples(SampleProducer* sampleBuffer) = 0;
};

