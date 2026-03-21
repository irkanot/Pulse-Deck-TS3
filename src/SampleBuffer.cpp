// src/SampleBuffer.cpp
//----------------------------------
// Pulse Deck TS3 Source Code
// Copyright (c) 2015 Davide (Irkanot)
// All rights reserved
// Contact: support@pulse-deck-ts3.local
//----------------------------------

#include <algorithm>
#include <cstring>
#include "SampleBuffer.h"


SampleBuffer::SampleBuffer(int channels, size_t maxSize /*= 0*/) :
	m_channels(channels),
	m_maxSize(maxSize),
	m_cbProd(nullptr),
	m_cbCons(nullptr)
{
}


void SampleBuffer::produce(const short* samples, int count)
{
	assert(!m_mutex.try_lock() && "Mutex not locked");
	if (m_maxSize == 0 || avail() < (int)m_maxSize)
	{
		m_buf.insert(m_buf.end(), samples, samples + (count * m_channels));
		if (m_cbProd)
			m_cbProd->onProduceSamples(samples, count, this);
	}
}


int SampleBuffer::consume(short* samples, int maxCount, bool eraseConsumed)
{
	assert(!m_mutex.try_lock() && "Mutex not locked");
	int count = std::min(avail(), maxCount);
	size_t shorts = count * m_channels;
	if (samples)
		memcpy(samples, m_buf.data(), shorts * 2);
	if (eraseConsumed)
		m_buf.erase(m_buf.begin(), m_buf.begin() + count * m_channels);
	if (m_cbCons)
		m_cbCons->onConsumeSamples(samples, count, this);
	return count;
}

