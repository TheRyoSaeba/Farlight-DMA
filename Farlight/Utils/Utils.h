#pragma once
#define NOMINMAX
#include <cstdint>
#include <corecrt_math.h>
#include <d3d9.h>
#include <vector>
#include <string>
#include <DMALibrary/Memory/Memory.h>
 
#define M_PI 3.14159265359

struct Vector2 {
	float x = 0.0f;
	float y = 0.0f;

	Vector2() = default;
	Vector2(float x, float y) : x(x), y(y) {}
};


class Vector3
{
public:
	Vector3() : x(0.f), y(0.f), z(0.f)
	{

	}

	Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
	{

	}
	~Vector3()
	{

	}

	float x;
	float y;
	float z;

	inline float Dot(Vector3 v)
	{
		return x * v.x + y * v.y + z * v.z;
	}

	inline float Distance(Vector3 v)
	{
		float x = this->x - v.x;
		float y = this->y - v.y;
		float z = this->z - v.z;

		return sqrtf((x * x) + (y * y) + (z * z)) * 0.03048f;
	}

	Vector3 operator+(Vector3 v) const
	{
		return Vector3(x + v.x, y + v.y, z + v.z);
	}

	Vector3 operator-(Vector3 v) const
	{
		return Vector3(x - v.x, y - v.y, z - v.z);
	}

	Vector3 operator*(float number) const {
		return Vector3(x * number, y * number, z * number);
	}
};

 

template <class T>
class TArray
{
public:
	int Num() const
	{
		return count;
	}

	int getIdentifier()
	{
		return data + count * max;
	}

	bool isValid() const
	{
		if (count > max)
			return false;
		if (!data)
			return false;
		return true;
	}

	uint64_t getAddress() const
	{
		return data;
	}

	uintptr_t data;
	uint32_t count;
	uint32_t max;
};


struct FString {
	wchar_t* Data;
	int32_t Count;
	int32_t Max;
	bool IsValid() const {
		return Data != nullptr && Count > 0 && Max >= Count;
	}
};

struct FQuat
{
	float x;
	float y;
	float z;
	float w;
};

struct FVECTOR2D
{
	float x;
	float y;
};

struct FTransform
{
	FQuat rot;
	Vector3 translation;
	char pad[4];
	Vector3 scale;
	char pad1[4];

	D3DMATRIX ToMatrixWithScale()
	{
		D3DMATRIX m;
		m._41 = translation.x;
		m._42 = translation.y;
		m._43 = translation.z;

		float x2 = rot.x + rot.x;
		float y2 = rot.y + rot.y;
		float z2 = rot.z + rot.z;

		float xx2 = rot.x * x2;
		float yy2 = rot.y * y2;
		float zz2 = rot.z * z2;
		m._11 = (1.0f - (yy2 + zz2)) * scale.x;
		m._22 = (1.0f - (xx2 + zz2)) * scale.y;
		m._33 = (1.0f - (xx2 + yy2)) * scale.z;

		float yz2 = rot.y * z2;
		float wx2 = rot.w * x2;
		m._32 = (yz2 - wx2) * scale.z;
		m._23 = (yz2 + wx2) * scale.y;

		float xy2 = rot.x * y2;
		float wz2 = rot.w * z2;
		m._21 = (xy2 - wz2) * scale.y;
		m._12 = (xy2 + wz2) * scale.x;

		float xz2 = rot.x * z2;
		float wy2 = rot.w * y2;
		m._31 = (xz2 + wy2) * scale.z;
		m._13 = (xz2 - wy2) * scale.x;

		m._14 = 0.0f;
		m._24 = 0.0f;
		m._34 = 0.0f;
		m._44 = 1.0f;

		return m;
	}
};

struct FVector { float x, y, z; };
struct FRotator
{
	float Pitch;
	float Yaw;
	float Roll;

	FRotator() = default;
	FRotator(float pitch, float yaw, float roll) : Pitch(pitch), Yaw(yaw), Roll(roll) {}
};


struct Camera
{
	FVector Location;
	FRotator Rotation;
	char padding[0x30];
	float FOV;
};

namespace util {
	inline bool IsValidVA(uintptr_t address) {
		return (address && address >= 0x10000 && address <= 0x7FFFFFFFFFFF);
	}
}


enum class ETargetPriority {
	Head,
	Body
};

enum AimType {
	AIM_NONE,
	BPRO,
	NET,
	MEMORY
};

 

inline D3DMATRIX Matrix(Vector3 rot, Vector3 origin) {
	float radPitch = (rot.x * float(M_PI) / 180.f);
	float radYaw = (rot.y * float(M_PI) / 180.f);
	float radRoll = (rot.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}

inline D3DMATRIX MatrixMultiplication(D3DMATRIX pM1, D3DMATRIX pM2)
{
	D3DMATRIX pOut;
	pOut._11 = pM1._11 * pM2._11 + pM1._12 * pM2._21 + pM1._13 * pM2._31 + pM1._14 * pM2._41;
	pOut._12 = pM1._11 * pM2._12 + pM1._12 * pM2._22 + pM1._13 * pM2._32 + pM1._14 * pM2._42;
	pOut._13 = pM1._11 * pM2._13 + pM1._12 * pM2._23 + pM1._13 * pM2._33 + pM1._14 * pM2._43;
	pOut._14 = pM1._11 * pM2._14 + pM1._12 * pM2._24 + pM1._13 * pM2._34 + pM1._14 * pM2._44;
	pOut._21 = pM1._21 * pM2._11 + pM1._22 * pM2._21 + pM1._23 * pM2._31 + pM1._24 * pM2._41;
	pOut._22 = pM1._21 * pM2._12 + pM1._22 * pM2._22 + pM1._23 * pM2._32 + pM1._24 * pM2._42;
	pOut._23 = pM1._21 * pM2._13 + pM1._22 * pM2._23 + pM1._23 * pM2._33 + pM1._24 * pM2._43;
	pOut._24 = pM1._21 * pM2._14 + pM1._22 * pM2._24 + pM1._23 * pM2._34 + pM1._24 * pM2._44;
	pOut._31 = pM1._31 * pM2._11 + pM1._32 * pM2._21 + pM1._33 * pM2._31 + pM1._34 * pM2._41;
	pOut._32 = pM1._31 * pM2._12 + pM1._32 * pM2._22 + pM1._33 * pM2._32 + pM1._34 * pM2._42;
	pOut._33 = pM1._31 * pM2._13 + pM1._32 * pM2._23 + pM1._33 * pM2._33 + pM1._34 * pM2._43;
	pOut._34 = pM1._31 * pM2._14 + pM1._32 * pM2._24 + pM1._33 * pM2._34 + pM1._34 * pM2._44;
	pOut._41 = pM1._41 * pM2._11 + pM1._42 * pM2._21 + pM1._43 * pM2._31 + pM1._44 * pM2._41;
	pOut._42 = pM1._41 * pM2._12 + pM1._42 * pM2._22 + pM1._43 * pM2._32 + pM1._44 * pM2._42;
	pOut._43 = pM1._41 * pM2._13 + pM1._42 * pM2._23 + pM1._43 * pM2._33 + pM1._44 * pM2._43;
	pOut._44 = pM1._41 * pM2._14 + pM1._42 * pM2._24 + pM1._43 * pM2._34 + pM1._44 * pM2._44;

	return pOut;
}

 
inline Vector2 WorldToScreen(const Vector3& worldPos,
	const Camera& cam,
	int screenW,
	int screenH)
{
	D3DMATRIX rot = Matrix(
		Vector3(cam.Rotation.Pitch, cam.Rotation.Yaw, cam.Rotation.Roll),
		Vector3(0, 0, 0));

	Vector3 axisX = { rot.m[0][0], rot.m[0][1], rot.m[0][2] };
	Vector3 axisY = { rot.m[1][0], rot.m[1][1], rot.m[1][2] };
	Vector3 axisZ = { rot.m[2][0], rot.m[2][1], rot.m[2][2] };

	Vector3 delta = {
		worldPos.x - cam.Location.x,
		worldPos.y - cam.Location.y,
		worldPos.z - cam.Location.z
	};

	Vector3 viewSpace = {
		delta.Dot(axisX),
		delta.Dot(axisY),
		delta.Dot(axisZ)
	};

	if (viewSpace.z < 1.0f) viewSpace.z = 1.0f;

	float ScreenCenterX = screenW / 2.0f;
	float ScreenCenterY = screenH / 2.0f;

	float d = ScreenCenterY / tanf(cam.FOV * M_PI / 360.0f);
	float FovAngle = 2.0f * atan(ScreenCenterX / d) * 180.0f / M_PI;

	float screenX = ScreenCenterX + viewSpace.x * (ScreenCenterX / tanf(FovAngle * M_PI / 360.0f)) / viewSpace.z;
	float screenY = ScreenCenterY - viewSpace.y * (ScreenCenterX / tanf(FovAngle * M_PI / 360.0f)) / viewSpace.z;

	return { screenX, screenY };
}

inline Vector2 doMatrix(FTransform Bone, FTransform C2W, Camera camera, int screenWidth, int screenHeight) {
	D3DMATRIX matrix = MatrixMultiplication(Bone.ToMatrixWithScale(), C2W.ToMatrixWithScale());
	Vector3 boneWorld = Vector3(matrix._41, matrix._42, matrix._43);

	D3DMATRIX tempMatrix = Matrix(
		Vector3(camera.Rotation.Pitch, camera.Rotation.Yaw, camera.Rotation.Roll),
		Vector3(0, 0, 0));

	Vector3 vAxisX = Vector3(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
	Vector3 vAxisY = Vector3(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
	Vector3 vAxisZ = Vector3(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

	Vector3 vDelta = boneWorld - Vector3(camera.Location.x, camera.Location.y, camera.Location.z);

	Vector3 vTransformed = Vector3(
		vDelta.Dot(vAxisY),  
		vDelta.Dot(vAxisZ),    
		vDelta.Dot(vAxisX)   
	);

	if (vTransformed.z < 1.0f) vTransformed.z = 1.0f;

	float ScreenCenterX = screenWidth / 2.0f;
	float ScreenCenterY = screenHeight / 2.0f;


	float d = ScreenCenterY / tanf(camera.FOV * M_PI / 360.0f);
	float FovAngle = 2.0f * atan(ScreenCenterX / d) * 180.0f / M_PI;

	float screenX = ScreenCenterX + vTransformed.x * (ScreenCenterX / tanf(FovAngle * M_PI / 360.0f)) / vTransformed.z;
	float screenY = ScreenCenterY - vTransformed.y * (ScreenCenterX / tanf(FovAngle * M_PI / 360.0f)) / vTransformed.z;

	return Vector2(screenX, screenY);
}


 

inline float GetCrossDistance(double x1, double y1, double x2, double y2) {
	double dx = x2 - x1;
	double dy = y2 - y1;
	return sqrt(dx * dx + dy * dy);
}
 


 

 