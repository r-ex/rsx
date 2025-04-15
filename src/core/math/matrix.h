#pragma once

struct float3x4
{
	float m[3][4];
};

struct matrix3x4_t
{
	float* operator[](int i) { assert((i >= 0) && (i < 3)); return m_flMatVal[i]; }
	const float* operator[](int i) const { assert((i >= 0) && (i < 3)); return m_flMatVal[i]; }
	float* Base() { return &m_flMatVal[0][0]; }
	const float* Base() const { return &m_flMatVal[0][0]; }

	float m_flMatVal[3][4];
};


void MatrixAngles(const matrix3x4_t& matrix, float* angles); // !!!!

void MatrixGetColumn(const matrix3x4_t& in, int column, Vector& out);
void MatrixSetColumn(const Vector& in, int column, matrix3x4_t& out);

// https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/public/mathlib/mathlib.h#L857
inline void MatrixPosition(const matrix3x4_t& matrix, Vector& position)
{
	MatrixGetColumn(matrix, 3, position);
}

// https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/public/mathlib/mathlib.h#L872
inline void MatrixAngles(const matrix3x4_t& matrix, QAngle& angles)
{
	MatrixAngles(matrix, &angles.x);
}

inline void MatrixAngles(const matrix3x4_t& matrix, QAngle& angles, Vector& position)
{
	MatrixAngles(matrix, angles);
	MatrixPosition(matrix, position);
}

inline void MatrixAngles(const matrix3x4_t& matrix, RadianEuler& angles)
{
	MatrixAngles(matrix, &angles.x);

	angles.Init(DEG2RAD(angles.z), DEG2RAD(angles.x), DEG2RAD(angles.y));
}

void MatrixAngles(const matrix3x4_t& mat, RadianEuler& angles, Vector& position);

void MatrixAngles(const matrix3x4_t& mat, Quaternion& q, Vector& position);