#ifndef VECTOR2D_H
#define VECTOR2D_H
#include <cmath>
#include <cfloat>
#include <climits>
#include <tuple>

/*The Vector2d class is an object consisting of simply an x and
y value. Certain operators are overloaded to make it easier
for vector math to be performed.*/
template<typename T>
class Vector2d {
public:
	/*The x and y values are public to give easier access for
	outside funtions. Accessors and mutators are not really
	necessary*/
	T x;
	T y;

	//Constructor assigns the inputs to x and y.
	Vector2d() : x(T(0)), y(T(0)) {}
	Vector2d(T a, T b) : x(a), y(b) {}

	/*The following operators simply return Vector2ds that
	have operations performed on the relative (x, y) values*/
	Vector2d& operator+=(const Vector2d& v) { x += v.x; y += v.y; return *this; }
	Vector2d& operator-=(const Vector2d& v) { x -= v.x; y -= v.y; return *this; }
	Vector2d& operator*=(const Vector2d& v) { x *= v.x; y *= v.y; return *this; }
	Vector2d& operator/=(const Vector2d& v) { x /= v.x; y /= v.y; return *this; }

	//Check if the Vectors have the same values (uses pairwise comparison of 
	// 'std::tuple' on the x, y values of L and R.)
	friend bool operator==(const Vector2d& L, const Vector2d& R) {
		return std::tie(L.x, L.y) == std::tie(R.x, R.y);
	}
	friend bool operator!=(const Vector2d& L, const Vector2d& R) { return !(L == R); }

	void set(T a, T b) { x = a; y = b; }
	/*Check which Vectors are closer or further from the origin.*/
	friend bool operator>(const Vector2d& L, const Vector2d& R) { return LengthSq(L) < LengthSq(R); }
	friend bool operator>=(const Vector2d& L, const Vector2d& R) { return !(L > R); }
	friend bool operator<(const Vector2d& L, const Vector2d& R) { return R < L; }
	friend bool operator<=(const Vector2d& L, const Vector2d& R) { return !(R < L); }

	//Negate both the x and y values.
	Vector2d operator-() const { return Vector2d(-x, -y); }

	//Apply scalar operations.
	Vector2d& operator*(T s) { x *= s; y *= s; return *this; }
	Vector2d& operator/(T s) { x /= s; y /= s; return *this; }

	//Returns the length of the vector from the origin.
	double Length() const { return sqrt(x*x + y*y); }
	double LengthSq()const { return x*x + y*y; }

};

template<class T> Vector2d<T> operator*(const T& s, const Vector2d<T>& v) { return Vector2d<T>(v) *= s; }
template<class T> Vector2d<T> operator*(const Vector2d<T>& v, const T& s) { return Vector2d<T>(v) *= s; }

template<class T> Vector2d<T>  operator-(const Vector2d<T>& v1, const Vector2d<T>& v2) { return Vector2d<T>(v1.x-v2.x, v1.y-v2.y); }
template<class T> Vector2d<T>  operator+(const Vector2d<T>& v1, const Vector2d<T>& v2) { return Vector2d<T>(v1.x+v2.x, v1.y+v2.y); }

//Product functions
template<class T> T DotProduct(const Vector2d<T>& a, const Vector2d<T>& b) { return  ((a.x * b.x) + (a.y * b.y)); }
template<class T> T CrossProduct(const Vector2d<T>& a, const Vector2d<T>& b) { return ((a.x * b.y) - (a.y * b.x)); }

//Return the unit vector of the input
template<class T> Vector2d<T> Normal(const Vector2d<T>& a) { double mag = a.Length(); return Vector2d<T>(a.x / mag, a.y / mag); }

//Return a vector perpendicular to the left.
template<class T> Vector2d<T> Perpendicular(const Vector2d<T>& a) { return Vector2d<T>(a.y, -a.x); }
//Return true if two line segments intersect.
template<class T> 
bool Intersect(const Vector2d<T>&aa, const Vector2d<T>& ab, const Vector2d<T>& ba, const Vector2d<T>& bb)
{
	Vector2d<T> p = aa;
	Vector2d<T> r = ab - aa;
	Vector2d<T> q = ba;
	Vector2d<T> s = bb - ba;

	double t = CrossProduct((q - p), s) / CrossProduct(r, s);
	double u = CrossProduct((q - p), r) / CrossProduct(r, s);

	return (0.0 <= t && t <= 1.0) &&
		(0.0 <= u && u <= 1.0);
}

//Return the point where two lines intersect.
template<class T>
Vector2d<T> GetIntersect(const Vector2d<T>&aa, const Vector2d<T>& ab, const Vector2d<T>& ba, const Vector2d<T>& bb)
{
	double pX = (aa.x*ab.y - aa.y*ab.x)*(ba.x - bb.x) -
		(ba.x*bb.y - ba.y*bb.x)*(aa.x - ab.x);
	double pY = (aa.x*ab.y - aa.y*ab.x)*(ba.y - bb.y) -
		(ba.x*bb.y - ba.y*bb.x)*(aa.y - ab.y);
	double denominator = (aa.x - ab.x)*(ba.y - bb.y) -
		(aa.y - ab.y)*(ba.x - bb.x);

	return Vector2d(pX / denominator, pY / denominator);
}

#endif