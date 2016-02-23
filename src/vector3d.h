#ifndef VECTOR3D_H
#define VECTOR3D_H
#include <cmath>
#include <cfloat>
#include <climits>
#include <tuple>

/*The Vector3d class is an object consisting of simply an x and
y value. Certain operators are overloaded to make it easier
for vector math to be performed.*/
template<typename T>
class Vector3d
{
public:
	/*The x and y values are public to give easier access for
	outside funtions. Accessors and mutators are not really
	necessary*/
	T x;
	T y;
	T z;

	//Constructor assigns the inputs to x and y.
	Vector3d() : x(T(0)), y(T(0)), z(T(0)) {}
	Vector3d(T a, T b, T c) : x(a), y(b), z(c) {}

	/*The following operators simply return Vector3ds that
	have operations performed on the relative (x, y) values*/
	Vector3d& operator+=(const Vector3d& v) { x += v.x; y += v.y; z += v.z; return *this; }
	Vector3d& operator+=(const T a) { x += a; y += a; z += a; return *this; }
	Vector3d& operator-=(const Vector3d& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	Vector3d& operator-=(const T a) { x -= a; y -= a; z -= a; return *this; }
	Vector3d& operator*=(const Vector3d& v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
	Vector3d& operator*=(const T a) { x *= a; y *= a; z *= a; return *this; }
	Vector3d& operator/=(const Vector3d& v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
	Vector3d& operator/=(const T a) { x /= a; y /= a; z /= a; return *this; }

	//Check if the Vectors have the same values (uses pairwise comparison of 
	// 'std::tuple' on the x, y values of L and R.)
	friend bool operator==(const Vector3d& L, const Vector3d& R) {
		return std::tie(L.x, L.y, L.z) == std::tie(R.x, R.y, R.z);
	}
	friend bool operator!=(const Vector3d& L, const Vector3d& R) { return !(L == R); }

	void set(T a, T b, T c) { x = a; y = b; z = c; }
	/*Check which Vectors are closer or further from the origin.*/
	friend bool operator>(const Vector3d& L, const Vector3d& R) { return LengthSq(L) < LengthSq(R); }
	friend bool operator>=(const Vector3d& L, const Vector3d& R) { return !(L > R); }
	friend bool operator<(const Vector3d& L, const Vector3d& R) { return R < L; }
	friend bool operator<=(const Vector3d& L, const Vector3d& R) { return !(R < L); }

	//Negate both the x and y values.
	Vector3d operator-() const { return Vector3d(-x, -y, -z); }

	//Apply scalar operations.
	Vector3d operator*(T s) { return Vector3d<T>(*this) *= s; }
	Vector3d operator/(T s) { return Vector3d<T>(*this) /= s; }

	//Returns the length of the vector from the origin.
	double Length() const { return sqrt(x*x + y*y, z*z); }
	double LengthSq()const { return x*x + y*y + z*z; }

};

template<class T> Vector3d<T> operator*(const T s, const Vector3d<T>& v) { return Vector3d<T>(v) *= s; }
template<class T> Vector3d<T> operator*(const Vector3d<T>& v, const T s) { return Vector3d<T>(v) *= s; }

template<class T> Vector3d<T>  operator-(const Vector3d<T>& v1, const Vector3d<T>& v2) { return Vector3d<T>(v1) -= v2; }
template<class T> Vector3d<T>  operator+(const Vector3d<T>& v1, const Vector3d<T>& v2) { return Vector3d<T>(v1) += v2; }

template<class T> Vector3d<T>  operator/(const Vector3d<T>& v1, const T a) { return Vector3d<T>(v1) /= a; }

//Product functions
template<class T> T DotProduct(const Vector3d<T>& a, const Vector3d<T>& b) { return  ((a.x * b.x) + (a.y * b.y) + (a.z * b.z)); }
template<class T>  Vector3d<T>  CrossProduct(const Vector3d<T>& a, const Vector3d<T>& b) {
	return Vector3d<T>((a.y*b.z - a.z - b.y), (a.z*b.x - a.x*b.z), (a.x*b.y - a.y*b.x));
}

//Return the unit vector of the input
template<class T> Vector3d<T> Normal(const Vector3d<T>& a) { double mag = a.Length(); return v<T>(a.x / mag, a.y / mag, a.z / mag); }

//Return a vector perpendicular to the left.
//template<class T> Vector3d<T> Perpendicular(const Vector3d<T>& a) { return Vector3d<T>(a.y, -a.x); }
//Return true if two line segments intersect.
/*template<class T> bool Intersect(const Vector3d<T>&aa, const Vector3d<T>& ab, const Vector3d<T>& ba, const Vector3d<T>& bb)
{
Vector3d<T> p = aa;
Vector3d<T> r = ab - aa;
Vector3d<T> q = ba;
Vector3d<T> s = bb - ba;

double t = CrossProduct((q - p), s) / CrossProduct(r, s);
double u = CrossProduct((q - p), r) / CrossProduct(r, s);

return (0.0 <= t && t <= 1.0) &&
(0.0 <= u && u <= 1.0);
}
*/
//Return the point where two lines intersect.
/*template<class T> Vector3d<T> GetIntersect(const Vector3d<T>&aa, const Vector3d<T>& ab, const Vector3d<T>& ba, const Vector3d<T>& bb)
{
double pX = (aa.x*ab.y - aa.y*ab.x)*(ba.x - bb.x) -
(ba.x*bb.y - ba.y*bb.x)*(aa.x - ab.x);
double pY = (aa.x*ab.y - aa.y*ab.x)*(ba.y - bb.y) -
(ba.x*bb.y - ba.y*bb.x)*(aa.y - ab.y);
double denominator = (aa.x - ab.x)*(ba.y - bb.y) -
(aa.y - ab.y)*(ba.x - bb.x);

return Vector3d(pX / denominator, pY / denominator);
}
*/
#endif