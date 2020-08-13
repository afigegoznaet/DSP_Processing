#ifndef FILTERS_HPP
#define FILTERS_HPP

/**
This is a modified version of ftools.h provided by Richard Dobson as part of
"The Audio Programming Book"
*/

/* Copyright (c) 2009 Richard Dobson

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

/* ftools.h : a set of first/second order recursive filters */
/* SOURCES:

  (1) Robert Bristow-Johnson "EQ Cookbook"  (ref. music-dsp.org )

  (2) Dodge & Jerse "Computer Music" 2nd Ed, pp 209-219

	All filter tick functions support time-varying parameters.
	There is no attempt here to optimise the code!

  Functions are provided to enable display of filter coefficients,
  and to create a filter using externally supplied coefficients.
  No time-varying control possible in this case!
*/

#ifndef FILTERS_H_INCLUDED
#define FILTERS_H_INCLUDED
#define _USE_MATH_DEFINES
#include <vector>
enum BW_FilterType { LOWPASS,
					 HIPASS,
					 BANDPASS,
					 BANDREJECT };

struct BQ_Coeff {
	double a0, a1, a2, b1, b2;
};


class BW_Filter {
public:
	void					   init(BW_FilterType fType, unsigned long samplingRate,
									double cutoffFreq, double bandWidth, int filtRespLength = 20000);
	int						   getTick(int input);
	const std::vector<double> &getFrequencyResponse() const { return freqResp; }
	const std::vector<double> &getPhaseResponse() const { return phaseResp; }

private:
	void updateLowPass();
	void updateHighPass();
	void updateBandPass();
	void updateBandReject();
	void calculateFilterResponse();

private:
	BQ_Coeff			coeffs;
	double				x1{}, x2{}, y1{}, y2{};
	double				cutoffFreq{}, samplingRate{}, bandWidth{};
	double				piDivSr{}, twoPiDivSr{};
	BW_FilterType		fType;
	std::vector<double> freqResp{};
	std::vector<double> phaseResp{};
};


#endif // FILTERS_H_INCLUDED

#endif // FILTERS_HPP
