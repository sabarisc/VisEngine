#pragma once
#include <cmath>

struct Vector3
{
	union
	{
		float v[3];
		struct
		{
			float x;
			float y;
			float z;
		};
	};
};

struct Vector2
{
	union
	{
		float v[2];
		struct
		{
			float x;
			float y;
		};
	};
};

struct Matrix43
{
	union
	{
		float m_43[4][3];
		Vector3 m_4[4];
	};
	void SetIdentity()
	{
		m_43[0][0] = 1.0f;
		m_43[0][1] = 0.0f;
		m_43[0][2] = 0.0f;
		m_43[1][0] = 0.0f;
		m_43[1][1] = 1.0f;
		m_43[1][2] = 0.0f;
		m_43[2][0] = 0.0f;
		m_43[2][1] = 0.0f;
		m_43[2][2] = 1.0f;
		m_43[3][0] = 0.0f;
		m_43[3][1] = 0.0f;
		m_43[3][2] = 0.0f;
	}
};

inline void Vector3ScaledAdd( Vector3 &origin, float scale, Vector3 dir )
{
	origin.x += dir.x * scale;
	origin.y += dir.y * scale;
	origin.z += dir.z * scale;
}

inline void Vector3Copy( Vector3 from, Vector3 &to )
{
	to.x = from.x;
	to.y = from.y;
	to.z = from.z;
}

inline void Vector3Clear( Vector3 &vec )
{
	vec.x = vec.y = vec.z = 0.0f;
}