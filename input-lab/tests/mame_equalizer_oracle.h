// SPDX-License-Identifier: BSD-3-Clause
// Unmodified coefficient/history methods extracted from MAMEPPK's MAME 0.289 EQ.
// Copyright (c) Olivier Galibert. Test adapter (c) 2026 Input Lab contributors.
// Origin SHA256 eq.cpp: 80046ef4bccf6476755a82b7b3ef616de8aeb7f78f10c5c2d463d585a581598b
// See licenses/mame-equalizer.txt. This is an external reference, not the port DSP.
#pragma once
#include "../../port/equalizer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>
using u32 = uint32_t; using s32 = int32_t;
using std::abs; using std::pow; using std::tan; using std::sqrt;
inline bool BIT(u32 v, unsigned bit) { return v & (1u << bit); }
class MameEqualizerOracle {
public:
static constexpr int BANDS=5;
	struct history {
		float m_v0, m_v1, m_v2;
		history() { m_v0 = m_v1 = m_v2 = 0; }
		void push(float v) { m_v2 = m_v1; m_v1 = m_v0; m_v0 = v; }
	};

	struct filter {
		float m_a1, m_a2, m_b0, m_b1, m_b2;
		void clear() { m_a1 = 0; m_a2 = 0; m_b0 = 1; m_b1 = 0; m_b2 = 0; }
		void apply(history &x, history &y) const {
			y.push(m_b0 * x.m_v0 + m_b1 * x.m_v1 + m_b2 * x.m_v2 - m_a1 * y.m_v0 - m_a2 * y.m_v1);
		}
	};


u32 m_mode=1, m_band_mask=0, m_sample_rate;
u32 m_f[BANDS]; float m_q[BANDS], m_db[BANDS];
bool m_low_shelf, m_high_shelf;
std::array<filter,BANDS> m_filter;
std::array<std::array<history,BANDS+1>,2> m_history;
MameEqualizerOracle(int rate,const port::EqualizerSettings &s):m_sample_rate(rate),m_low_shelf(s.lowShelf),m_high_shelf(s.highShelf) {
 m_mode=s.active;
 for(int b=0;b<BANDS;++b){m_f[b]=s.frequency[b];m_q[b]=s.q[b];m_db[b]=s.gain[b];build_filter(b);}
}
void build_filter(u32);void build_low_shelf(u32);void build_high_shelf(u32);void build_peak(u32);
std::vector<float> Apply(const std::vector<float> &src) {
 if(!m_mode||!m_band_mask)return src;
 auto dest=src;
 for(unsigned c=0;c<2;++c)for(size_t n=c;n<src.size();n+=2){
   m_history[c][0].push(src[n]);
   for(unsigned b=0;b<BANDS;++b)m_filter[b].apply(m_history[c][b],m_history[c][b+1]);
   dest[n]=m_history[c][BANDS].m_v0;
 }
 return dest;
}
};
void MameEqualizerOracle::build_filter(u32 band)
{
	if(s32(roundf(m_db[band] * 10.0f)) == 0)
		m_band_mask &= ~(1 << band);
	else
		m_band_mask |= 1 << band;

	if(band == 0 && m_low_shelf) {
		build_low_shelf(band);
		return;
	}
	if(band == BANDS-1 && m_high_shelf) {
		build_high_shelf(band);
		return;
	}
	build_peak(band);
}

void MameEqualizerOracle::build_low_shelf(u32 band)
{
	auto &fi = m_filter[band];
	if(!BIT(m_band_mask, band)) {
		fi.clear();
		return;
	}

	float sr = m_sample_rate;
	float f = std::clamp(float(m_f[band]), 1.0f, sr/2.0f - 1.0f);

	float V = pow(10, abs(m_db[band])/20);
	float K = tan(std::numbers::pi * f / sr);
	float K2 = K*K;

	constexpr double SQRT2 = std::numbers::sqrt2;
	if(m_db[band] > 0) {
		float d = 1 + SQRT2*K + K2;
		fi.m_b0 = (1 + sqrt(2*V)*K + V*K2)/d;
		fi.m_b1 = 2*(V*K2-1)/d;
		fi.m_b2 = (1 - sqrt(2*V)*K + V*K2)/d;
		fi.m_a1 = 2*(K2-1)/d;
		fi.m_a2 = (1 - SQRT2*K + K2)/d;
	} else {
		float d = 1 + sqrt(2*V)*K + V*K2;
		fi.m_b0 = (1 + SQRT2*K + K2)/d;
		fi.m_b1 = 2*(K2-1)/d;
		fi.m_b2 = (1 - SQRT2*K + K2)/d;
		fi.m_a1 = 2*(V*K2-1)/d;
		fi.m_a2 = (1 - sqrt(2*V)*K + V*K2)/d;
	}
}

void MameEqualizerOracle::build_high_shelf(u32 band)
{
	auto &fi = m_filter[band];
	if(!BIT(m_band_mask, band)) {
		fi.clear();
		return;
	}

	float sr = m_sample_rate;
	float f = std::clamp(float(m_f[band]), 1.0f, sr/2.0f - 1.0f);

	float V = pow(10, m_db[band]/20);
	float K = tan(std::numbers::pi * f / sr);
	float K2 = K*K;

	constexpr double SQRT2 = std::numbers::sqrt2;
	if(m_db[band] > 0) {
		float d = 1 + SQRT2*K + K2;
		fi.m_b0 = (V + sqrt(2*V)*K + K2)/d;
		fi.m_b1 = 2*(K2-V)/d;
		fi.m_b2 = (V - sqrt(2*V)*K + K2)/d;
		fi.m_a1 = 2*(K2-1)/d;
		fi.m_a2 = (1 - SQRT2*K + K2)/d;
	} else {
		float d = 1 + sqrt(2*V)*K + V*K2;
		fi.m_b0 = V*(1 + SQRT2*K + K2)/d;
		fi.m_b1 = 2*V*(K2-1)/d;
		fi.m_b2 = V*(1 - SQRT2*K + K2)/d;
		fi.m_a1 = 2*(V*K2-1)/d;
		fi.m_a2 = (1 - sqrt(2*V)*K + V*K2)/d;
	}
}

void MameEqualizerOracle::build_peak(u32 band)
{
	auto &fi = m_filter[band];
	if(!BIT(m_band_mask, band)) {
		fi.clear();
		return;
	}

	float sr = m_sample_rate;
	float f = std::clamp(float(m_f[band]), 1.0f, sr/2.0f - 1.0f);

	float V = pow(10, m_db[band]/20);
	float K = tan(std::numbers::pi * f / sr);
	float K2 = K*K;
	float Q = m_q[band];

	if(m_db[band] > 0) {
		float d = 1 + K/Q + K2;
		fi.m_b0 = (1 + V*K/Q + K2)/d;
		fi.m_b1 = 2*(K2-1)/d;
		fi.m_b2 = (1 - V*K/Q + K2)/d;
		fi.m_a1 = fi.m_b1;
		fi.m_a2 = (1 - K/Q + K2)/d;
	} else {
		float d = 1 + K/(V*Q) + K2;
		fi.m_b0 = (1 + K/Q + K2)/d;
		fi.m_b1 = 2*(K2-1)/d;
		fi.m_b2 = (1 - K/Q + K2)/d;
		fi.m_a1 = fi.m_b1;
		fi.m_a2 = (1 - K/(V*Q) + K2)/d;
	}
}


