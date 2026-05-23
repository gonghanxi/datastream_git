/**
 *
 * "A Collection of Useful C++ Classes for Digital Signal Processing"
 * By Vinnie Falco and Bernd Porr
 *
 * Official project location:
 * https://github.com/berndporr/iir1
 *
 * See Documentation.cpp for contact information, notes, and bibliography.
 * 
 * -----------------------------------------------------------------
 *
 * License: MIT License (http://www.opensource.org/licenses/mit-license.php)
 * Copyright (c) 2009 by Vinnie Falco
 * Copyright (c) 2011 by Bernd Porr
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 **/

#include "Common.h"
#include "PoleFilter.h"

namespace Iir {

//------------------------------------------------------------------------------


static const char cutoffError[] = "The cutoff frequency needs to be below the Nyquist frequency.";
static const char cutoffNeg[] = "Cutoff frequency is negative.";

complex_t LowPassTransform::transform (complex_t c)
{
  if (c == infinity())
    return complex_t (-1, 0);

  // frequency transform
  c = f * c; 
  
  // bilinear low pass transform
  return (1. + c) / (1. - c);
}

LowPassTransform::LowPassTransform (double fc,
                                    LayoutBase& digital,
                                    LayoutBase const& analog)
{

	if (!(fc < 0.5)) throw_invalid_argument(cutoffError);
	if (fc < 0.0) throw_invalid_argument(cutoffNeg);
	
	digital.reset ();

	// prewarp
    f = tan (doublePi * fc);//补偿双线性变换引起的频率畸变：
	
	const int numPoles = analog.getNumPoles ();
	const int pairs = numPoles / 2;
	for (int i = 0; i < pairs; ++i)
	{
		const PoleZeroPair& pair = analog[i];
        digital.addPoleZeroConjugatePairs (transform (pair.poles.first),  //将零极点进行低通的的双线性变换
						   transform (pair.zeros.first));
	}
	
	if (numPoles & 1)
	{
		const PoleZeroPair& pair = analog[pairs];
		digital.add (transform (pair.poles.first),
			     transform (pair.zeros.first));
	}
	
	digital.setNormal (analog.getNormalW(),
			   analog.getNormalGain());
}

//------------------------------------------------------------------------------

complex_t HighPassTransform::transform (complex_t c)
{
  if (c == infinity())
    return complex_t (1, 0);

  // frequency transform
  c = f * c; 

  // bilinear high pass transform
  return - (1. + c) / (1. - c);
}

HighPassTransform::HighPassTransform (double fc,
                                      LayoutBase& digital,
                                      LayoutBase const& analog)
{
	if (!(fc < 0.5)) throw_invalid_argument(cutoffError);
	if (fc < 0.0) throw_invalid_argument(cutoffNeg);
	
	digital.reset ();
	
	// prewarp
	f = 1. / tan (doublePi * fc);
	
	const int numPoles = analog.getNumPoles ();
	const int pairs = numPoles / 2;
	for (int i = 0; i < pairs; ++i)
	{
		const PoleZeroPair& pair = analog[i];
		digital.addPoleZeroConjugatePairs (transform (pair.poles.first),
						   transform (pair.zeros.first));
	}
	
	if (numPoles & 1)
	{
		const PoleZeroPair& pair = analog[pairs];
		digital.add (transform (pair.poles.first),
			     transform (pair.zeros.first));
	}
	
	digital.setNormal (doublePi - analog.getNormalW(),
			   analog.getNormalGain());
}

//------------------------------------------------------------------------------

// 带通滤波器变换的构造函数
// fc: 中心频率 (归一化频率，范围0-0.5)
// fw: 带宽 (归一化频率)
// digital: 目标数字滤波器结构(引用)
// analog: 源模拟滤波器原型(常量引用)
BandPassTransform::BandPassTransform (double fc,
                                      double fw,
                                      LayoutBase& digital,
                                      LayoutBase const& analog)
{
//    模拟原型→频带变换→双线性变换→频率预补偿
    // 参数有效性检查：确保中心频率小于奈奎斯特频率(0.5)
	if (!(fc < 0.5)) throw_invalid_argument(cutoffError);
    if (fc < 0.0) throw_invalid_argument(cutoffNeg); // 参数有效性检查：确保中心频率非负

    digital.reset (); // 重置目标数字滤波器结构，准备构建新滤波器
	
    // 将带宽转换为角频率（弧度/样本）
       // doublePi = 2π，表示完整数字频率范围
	const double ww = 2 * doublePi * fw;
	
    // pre-calcs

    // 计算带通滤波器的两个边界频率（预扭曲补偿）:
     // --------------------------------------------------
     // 计算带通下边界频率（ω_c2）
     // 公式：ω_c2 = 2πfc - (带宽/2)
	wc2 = 2 * doublePi * fc - (ww / 2);

    // 计算带通上边界频率（ω_c）
    // 公式：ω_c = ω_c2 + 带宽
	wc  = wc2 + ww;
	
	// what is this crap?

    // 边界保护：防止数值不稳定
    // --------------------------------------------------
    // 确保下边界不小于最小阈值(避免除零错误)
	if (wc2 < 1e-8)
		wc2 = 1e-8;

     // 确保上边界不超过最大阈值(小于π)
	if (wc  > doublePi-1e-8)
		wc  = doublePi-1e-8;
	
    // 计算双线性变换的辅助参数（频率预扭曲）
    // --------------------------------------------------
    // 参数a的计算（基于带通上下边界）
    // 公式：a = cos((ω_c + ω_c2)/2) / cos((ω_c - ω_c2)/2)

	a =     cos ((wc + wc2) * 0.5) /
		cos ((wc - wc2) * 0.5);
    // 参数b的计算（带宽相关）
    // 公式：b = 1 / tan((ω_c - ω_c2)/2)
	b = 1 / tan ((wc - wc2) * 0.5);
     // 计算参数的平方和乘积（加速后续计算）
	a2 = a * a;
	b2 = b * b;
	ab = a * b;
	ab_2 = 2 * ab;
    // 源模拟滤波器的极点数
	const int numPoles = analog.getNumPoles ();
    // 计算共轭复数极点对的数量（实数滤波器是复数共轭成对出现的）
	const int pairs = numPoles / 2;

    // 遍历处理每个复数极点对
    // --------------------------------------------------
	for (int i = 0; i < pairs; ++i)
	{
        const PoleZeroPair& pair = analog[i];  // 从模拟原型中获取第i个极零点对
        ComplexPair p1 = transform (pair.poles.first);  // 将模拟极点转换到数字域（带通变换）
        ComplexPair z1 = transform (pair.zeros.first);  // 将模拟零点转换到数字域（带通变换）
		
		digital.addPoleZeroConjugatePairs (p1.first, z1.first);
		digital.addPoleZeroConjugatePairs (p1.second, z1.second);
	}
	
     // 处理奇数阶情况（单个实数极零点）
	if (numPoles & 1)
	{
        ComplexPair poles = transform (analog[pairs].poles.first);  // 获取最后一个实数极点（不成对）
        ComplexPair zeros = transform (analog[pairs].zeros.first); // 获取对应的实数零点
		
        digital.add (poles, zeros);  // 添加实数极零点到数字滤波器
	}
    // 设置数字滤波器的归一化参数
    // --------------------------------------------------
    // 获取模拟原型的归一化频率
	double wn = analog.getNormalW();

    // 计算并设置数字滤波器的归一化频率
    // 公式：2 * atan(sqrt(tan((ω_c + ω_n)/2) * tan((ω_c2 + ω_n)/2)))

	digital.setNormal (2 * atan (sqrt (tan ((wc + wn)* 0.5) * tan((wc2 + wn)* 0.5))),
			   analog.getNormalGain());
}

ComplexPair BandPassTransform::transform (complex_t c)
{
	if (c == infinity())
		return ComplexPair (-1, 1);
	
	c = (1. + c) / (1. - c); // bilinear
	
	complex_t v = 0;
	v = addmul (v, 4 * (b2 * (a2 - 1) + 1), c);
	v += 8 * (b2 * (a2 - 1) - 1);
	v *= c;
	v += 4 * (b2 * (a2 - 1) + 1);
	v = std::sqrt (v);
	
	complex_t u = -v;
	u = addmul (u, ab_2, c);
	u += ab_2;
	
	v = addmul (v, ab_2, c);
	v += ab_2;
	
	complex_t d = 0;
	d = addmul (d, 2 * (b - 1), c) + 2 * (1 + b);
	
	return ComplexPair (u/d, v/d);
}

//------------------------------------------------------------------------------

BandStopTransform::BandStopTransform (double fc,
                                      double fw,
                                      LayoutBase& digital,
                                      LayoutBase const& analog)
{
	if (!(fc < 0.5)) throw_invalid_argument(cutoffError);
	if (fc < 0.0) throw_invalid_argument(cutoffNeg);

	digital.reset ();
	
	const double ww = 2 * doublePi * fw;
	
	wc2 = 2 * doublePi * fc - (ww / 2);
	wc  = wc2 + ww;
	
	// this is crap
	if (wc2 < 1e-8)
		wc2 = 1e-8;
	if (wc  > doublePi-1e-8)
		wc  = doublePi-1e-8;
	
	a = cos ((wc + wc2) * .5) /
		cos ((wc - wc2) * .5);
	b = tan ((wc - wc2) * .5);
	a2 = a * a;
	b2 = b * b;
	
	const int numPoles = analog.getNumPoles ();
	const int pairs = numPoles / 2;
	for (int i = 0; i < pairs; ++i)
	{
		const PoleZeroPair& pair = analog[i];
		ComplexPair p  = transform (pair.poles.first);
		ComplexPair z  = transform (pair.zeros.first);
		
		// trick to get the conjugate
		if (z.second == z.first)
			z.second = std::conj (z.first);
		
		digital.addPoleZeroConjugatePairs (p.first, z.first);
		digital.addPoleZeroConjugatePairs (p.second, z.second);
	}
	
	if (numPoles & 1)
	{
		ComplexPair poles = transform (analog[pairs].poles.first);
		ComplexPair zeros = transform (analog[pairs].zeros.first);
		
		digital.add (poles, zeros);
	}
	
	if (fc < 0.25)
		digital.setNormal (doublePi, analog.getNormalGain());
	else
		digital.setNormal (0, analog.getNormalGain());
}

ComplexPair BandStopTransform::transform (complex_t c)
{
	if (c == infinity())
		c = -1;
	else
		c = (1. + c) / (1. - c); // bilinear
	
	complex_t u (0);
	u = addmul (u, 4 * (b2 + a2 - 1), c);
	u += 8 * (b2 - a2 + 1);
	u *= c;
	u += 4 * (a2 + b2 - 1);
	u = std::sqrt (u);
	
	complex_t v = u * -.5;
	v += a;
	v = addmul (v, -a, c);
	
	u *= .5;
	u += a;
	u = addmul (u, -a, c);
	
	complex_t d (b + 1);
	d = addmul (d, b-1, c);
	
	return ComplexPair (u/d, v/d);
}
	
}
