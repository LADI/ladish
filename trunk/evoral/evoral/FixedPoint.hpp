/* Fixed point number class
 * Copyright (C) 2008 Dave Robillard <drobilla.net>
 * Copyright (C) 2006 Henry Strickland & Ryan Seto
 *
 * <http://www.opensource.org/licenses/mit-license.php>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/** T = underlying integer type
 *  F = number of bits for fractional portion (right of point)
 *  e.g. 32-bit 16:16 fixed point number = Fixed<uint32_t, 16>
 *       32-bit  8:24 fixed point number = Fixed<uint32_t, 24>
 */
template<T,I>
class Fixed {
public:
	Fixed() : g(0) {}
	Fixed(const Fixed& a) : g( a.g ) {}
	Fixed(float a) : g( int(a / (float)STEP()) ) {}
	Fixed(double a) : g( int(a / (double)STEP()) ) {}
	Fixed(int a) : g( a << F ) {}
	Fixed(long a) : g( a << F ) {}
	Fixed& operator =(const Fixed& a) { g= a.g; return *this; }
	Fixed& operator =(float a) { g= Fixed(a).g; return *this; }
	Fixed& operator =(double a) { g= Fixed(a).g; return *this; }
	Fixed& operator =(int a) { g= Fixed(a).g; return *this; }
	Fixed& operator =(long a) { g= Fixed(a).g; return *this; }

	operator float()  { return g * (float)STEP(); }
	operator double() { return g * (double)STEP(); }
	operator int()    { return g>>F; }
	operator long()   { return g>>F; }

	Fixed operator +() const { return Fixed(RAW,g); }
	Fixed operator -() const { return Fixed(RAW,-g); }

	Fixed operator +(const Fixed& a) const { return Fixed(RAW, g + a.g); }
	Fixed operator -(const Fixed& a) const { return Fixed(RAW, g - a.g); }
#if 1
	// more acurate, using long long
	Fixed operator *(const Fixed& a) const { return Fixed(RAW,  (int)( ((long long)g * (long long)a.g ) >> F)); }
#else
	// faster, but with only half as many bits right of binary point
	Fixed operator *(const Fixed& a) const { return Fixed(RAW, (g>>Fhalf) * (a.g>>Fhalf) ); }
#endif
	Fixed operator /(const Fixed& a) const { return Fixed(RAW, int( (((long long)g << F2) / (long long)(a.g)) >> F) ); }

	Fixed operator +(float a) const { return Fixed(RAW, g + Fixed(a).g); }
	Fixed operator -(float a) const { return Fixed(RAW, g - Fixed(a).g); }
	Fixed operator *(float a) const { return Fixed(RAW, (g>>Fhalf) * (Fixed(a).g>>Fhalf) ); }
	Fixed operator /(float a) const { return Fixed(RAW, int( (((long long)g << F2) / (long long)(Fixed(a).g)) >> F) ); }

	Fixed operator +(double a) const { return Fixed(RAW, g + Fixed(a).g); }
	Fixed operator -(double a) const { return Fixed(RAW, g - Fixed(a).g); }
	Fixed operator *(double a) const { return Fixed(RAW, (g>>Fhalf) * (Fixed(a).g>>Fhalf) ); }
	Fixed operator /(double a) const { return Fixed(RAW, int( (((long long)g << F2) / (long long)(Fixed(a).g)) >> F) ); }

	Fixed& operator +=(Fixed a) { return *this = *this + a; return *this; }
	Fixed& operator -=(Fixed a) { return *this = *this - a; return *this; }
	Fixed& operator *=(Fixed a) { return *this = *this * a; return *this; }
	Fixed& operator /=(Fixed a) { return *this = *this / a; return *this; }

	Fixed& operator +=(int a) { return *this = *this + (Fixed)a; return *this; }
	Fixed& operator -=(int a) { return *this = *this - (Fixed)a; return *this; }
	Fixed& operator *=(int a) { return *this = *this * (Fixed)a; return *this; }
	Fixed& operator /=(int a) { return *this = *this / (Fixed)a; return *this; }

	Fixed& operator +=(long a) { return *this = *this + (Fixed)a; return *this; }
	Fixed& operator -=(long a) { return *this = *this - (Fixed)a; return *this; }
	Fixed& operator *=(long a) { return *this = *this * (Fixed)a; return *this; }
	Fixed& operator /=(long a) { return *this = *this / (Fixed)a; return *this; }

	Fixed& operator +=(float a) { return *this = *this + a; return *this; }
	Fixed& operator -=(float a) { return *this = *this - a; return *this; }
	Fixed& operator *=(float a) { return *this = *this * a; return *this; }
	Fixed& operator /=(float a) { return *this = *this / a; return *this; }

	Fixed& operator +=(double a) { return *this = *this + a; return *this; }
	Fixed& operator -=(double a) { return *this = *this - a; return *this; }
	Fixed& operator *=(double a) { return *this = *this * a; return *this; }
	Fixed& operator /=(double a) { return *this = *this / a; return *this; }

	bool operator ==(const Fixed& a) const { return g == a.g; }
	bool operator !=(const Fixed& a) const { return g != a.g; }
	bool operator <=(const Fixed& a) const { return g <= a.g; }
	bool operator >=(const Fixed& a) const { return g >= a.g; }
	bool operator  <(const Fixed& a) const { return g  < a.g; }
	bool operator  >(const Fixed& a) const { return g  > a.g; }

	bool operator ==(float a) const { return g == Fixed(a).g; }
	bool operator !=(float a) const { return g != Fixed(a).g; }
	bool operator <=(float a) const { return g <= Fixed(a).g; }
	bool operator >=(float a) const { return g >= Fixed(a).g; }
	bool operator  <(float a) const { return g  < Fixed(a).g; }
	bool operator  >(float a) const { return g  > Fixed(a).g; }

	bool operator ==(double a) const { return g == Fixed(a).g; }
	bool operator !=(double a) const { return g != Fixed(a).g; }
	bool operator <=(double a) const { return g <= Fixed(a).g; }
	bool operator >=(double a) const { return g >= Fixed(a).g; }
	bool operator  <(double a) const { return g  < Fixed(a).g; }
	bool operator  >(double a) const { return g  > Fixed(a).g; }

private:
	T g;

	const static int F2    = F * 2;
	const static int Fhalf = F/2;

	/** Smallest step we can represent */
	static double STEP() { return 1.0 / (1<<F); }

	enum FixedRaw { RAW };
	
	/** Private direct constructor from 'guts' */
	Fixed(FixedRaw, T guts) : g(guts) {}
};

inline Fixed operator +(float a, const Fixed& b) { return Fixed(a)+b; }
inline Fixed operator -(float a, const Fixed& b) { return Fixed(a)-b; }
inline Fixed operator *(float a, const Fixed& b) { return Fixed(a)*b; }
inline Fixed operator /(float a, const Fixed& b) { return Fixed(a)/b; }

inline bool operator ==(float a, const Fixed& b) { return Fixed(a) == b; }
inline bool operator !=(float a, const Fixed& b) { return Fixed(a) != b; }
inline bool operator <=(float a, const Fixed& b) { return Fixed(a) <= b; }
inline bool operator >=(float a, const Fixed& b) { return Fixed(a) >= b; }
inline bool operator  <(float a, const Fixed& b) { return Fixed(a)  < b; }
inline bool operator  >(float a, const Fixed& b) { return Fixed(a)  > b; }

inline Fixed operator +(double a, const Fixed& b) { return Fixed(a)+b; }
inline Fixed operator -(double a, const Fixed& b) { return Fixed(a)-b; }
inline Fixed operator *(double a, const Fixed& b) { return Fixed(a)*b; }
inline Fixed operator /(double a, const Fixed& b) { return Fixed(a)/b; }

inline bool operator ==(double a, const Fixed& b) { return Fixed(a) == b; }
inline bool operator !=(double a, const Fixed& b) { return Fixed(a) != b; }
inline bool operator <=(double a, const Fixed& b) { return Fixed(a) <= b; }
inline bool operator >=(double a, const Fixed& b) { return Fixed(a) >= b; }
inline bool operator  <(double a, const Fixed& b) { return Fixed(a)  < b; }
inline bool operator  >(double a, const Fixed& b) { return Fixed(a)  > b; }

int& operator +=(int& a, const Fixed& b) { a = (Fixed)a + b; return a; }
int& operator -=(int& a, const Fixed& b) { a = (Fixed)a - b; return a; }
int& operator *=(int& a, const Fixed& b) { a = (Fixed)a * b; return a; }
int& operator /=(int& a, const Fixed& b) { a = (Fixed)a / b; return a; }

long& operator +=(long& a, const Fixed& b) { a = (Fixed)a + b; return a; }
long& operator -=(long& a, const Fixed& b) { a = (Fixed)a - b; return a; }
long& operator *=(long& a, const Fixed& b) { a = (Fixed)a * b; return a; }
long& operator /=(long& a, const Fixed& b) { a = (Fixed)a / b; return a; }

float& operator +=(float& a, const Fixed& b) { a = a + b; return a; }
float& operator -=(float& a, const Fixed& b) { a = a - b; return a; }
float& operator *=(float& a, const Fixed& b) { a = a * b; return a; }
float& operator /=(float& a, const Fixed& b) { a = a / b; return a; }

double& operator +=(double& a, const Fixed& b) { a = a + b; return a; }
double& operator -=(double& a, const Fixed& b) { a = a - b; return a; }
double& operator *=(double& a, const Fixed& b) { a = a * b; return a; }
double& operator /=(double& a, const Fixed& b) { a = a / b; return a; }

