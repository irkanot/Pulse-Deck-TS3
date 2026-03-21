// src/SampleProducer.h
//----------------------------------
// Pulse Deck TS3 Source Code
// Copyright (c) 2015 Davide (Irkanot)
// All rights reserved
// Contact: support@pulse-deck-ts3.local
//----------------------------------

#pragma once

class SampleProducer
{
  public:
	virtual void produce(const short* samples, int count) = 0;
};

