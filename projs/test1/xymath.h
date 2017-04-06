#pragma once
#include <cassert>
#include <cmath>
#include <array>
#include <limits>
#include "xx_structs.h"
#include "xx_list.h"

using XY = xx::XY<float>;
struct XyMath
{
	static constexpr double pi = 3.14159265358979323846;
	static constexpr double digiNum = 100;							// ��ǰ��λ�� float ��, * 100 תΪ����
	static constexpr int xyTableDiameter = 2048						// ���ձ�ľ������(�뾶) Ϊ 10 ������
		, xyTableRadius = xyTableDiameter / 2
		, xyTableRadiusReduce = xyTableRadius - 1;
	static constexpr int angleCount = 256							// ��ǰ�ǰ� 1�ֽ� �ľ���������Ƕȵ�
		, halfAngleCount = angleCount / 2;

	std::array<XY, angleCount> xyIncs;
	std::array<uint8_t, xyTableDiameter * xyTableDiameter> angles;

	XyMath()
	{
		FillXyIncs();
		FillAngle();
	}

	// ���Ƕȶ�Ӧ�ı�׼����ֵ
	inline void FillXyIncs()
	{
		XY xyInc;
		for (int i = 0; i < angleCount; i++)
		{
			auto a = pi * i / halfAngleCount;
			xyInc.x = (float)std::cos(a);
			xyInc.y = (float)std::sin(a);
			xyIncs[i] = xyInc;
		}
	}

	// ���һ��Ƭ�������볯��ǶȵĶ�Ӧ
	inline void FillAngle()
	{
		for (int i = 0; i < xyTableDiameter; i++)
		{
			for (int j = 0; j < xyTableDiameter; j++)
			{
				auto idx = i * xyTableDiameter + j;

				auto x = j - xyTableRadius;
				auto y = i - xyTableRadius;

				auto v = (int)(std::atan2(i - xyTableRadius, j - xyTableRadius) / pi * halfAngleCount);
				if (v >= 0) angles[idx] = (uint8_t)v;
				else angles[idx] = (uint8_t)(angleCount + v);
			}
		}
	}

	inline uint8_t GetAngle(int x, int y)
	{
		// ��� x �� y ���� hlen ����Ҫ�� xy �ȱ���С
		auto x_ = std::abs(x);
		auto y_ = std::abs(y);
		if (x_ >= xyTableRadius || y_ >= xyTableRadius)
		{
			if (x_ >= y_)
			{
				y = (int)(y * (double)xyTableRadiusReduce / x_);
				x = x == x_ ? xyTableRadiusReduce : -xyTableRadiusReduce;
			}
			else
			{
				x = (int)(x * (double)xyTableRadiusReduce / y_);
				y = y == y_ ? xyTableRadiusReduce : -xyTableRadiusReduce;
			}
		}
		return angles[(y + xyTableRadius) * xyTableDiameter + (x + xyTableRadius)];
	}

	// ����ʸ����Ƕȳ���( ͨ������ target.xy - this.xy )
	// �� "��" �Ŵ� "����" ��������ȷ������( ͨ������ʸ�������ж���������ĳ��� )
	inline uint8_t GetAngle(XY xy)
	{
		return GetAngle((int)(xy.x * digiNum), (int)(xy.y * digiNum));
	}


	// ����ʸ����ȡ��׼����
	inline XY GetXyInc(XY xy)
	{
		return xyIncs[GetAngle(xy)];
	}
	inline XY GetXyInc(int x, int y)
	{
		return xyIncs[GetAngle(x, y)];
	}
	inline XY GetXyInc(uint8_t a)
	{
		return xyIncs[a];
	}


	// ��������
	inline static float GetDistance(int x, int y)
	{
		return sqrtf((float)(x * x + y * y));
	}
	inline static float GetDistance(XY xy)
	{
		return GetDistance((int)xy.x, (int)xy.y);
	}
	inline static float GetDistancePow2(XY a)
	{
		return a.x * a.x + a.y * a.y;
	}


	// �ͽ���һ���Ƕ�ƫת. ���ؽǶȽ��
	// a ÿ�β��������� inc �Ƕȵı仯, �ͽ��� b �ǶȽӽ�, �����ȫ����
	// a, b ��ֵ��ΧΪ 0 - 255, �Ҳ���ʱ�뷽��ת��. inc ����Ϊ������
	inline static uint8_t ChangeAngle(int a, int b, int inc)
	{
		if (a == b) return (uint8_t)a;
		else if (a > b)
		{
			if (a - b > halfAngleCount)
			{
				a += inc;
				if (a >= b + angleCount) return (uint8_t)b;
				return (uint8_t)a;
			}
			else
			{
				a -= inc;
				if (a <= b) return (uint8_t)b;
				return (uint8_t)a;
			}
		}
		else
		{
			if (b - a > halfAngleCount)
			{
				a -= inc;
				if (a <= b - angleCount) return (uint8_t)b;
				return (uint8_t)a;
			}
			else
			{
				a += inc;
				if (a >= b) return (uint8_t)b;
				return (uint8_t)a;
			}
		}
	}

	// ���� 0 -> pos ��ת a ֮����µ� pos
	inline XY RotateXY(XY pos, uint8_t a)
	{
		auto sincos = GetXyInc(a);
		auto sinA = sincos.y;
		auto cosA = sincos.x;
		return XY{ pos.x * cosA - pos.y * sinA, pos.x * sinA + pos.y * cosA };
	}

	// �Ƕ�תΪ 360 ������ϵ ���� 2 λ���ȵ�����
	inline static int ConvertToXZAngleForSend(uint8_t a)
	{
		return (int)(float((uint8_t)(-(int8_t)a + 64)) / 256.0f * 36000.0f);
	}

	// �� 360 �����ö���Ƕ�ֵ
	inline static uint8_t ConvertFromXZ360Angle(int a)
	{
		return (uint8_t)(-(int8_t)(uint8_t)(a / 360.0f * 256.0f) + 64);
	}

	// ���
	inline static float Dot(XY const& p1, XY const& p2)
	{
		XY p = p1 * p2;
		return p.x + p.y;
	}

	// ��������
	inline static float Inversesqrt(float const& x)
	{
		return 1.0f / sqrtf(x);
	}

	// ��һ��( �����Ͻ������� GetXyInc �����, ֻ�������Ƚϵ�, �ֳ����� )
	inline static XY Normalize(XY const& p)
	{
		return p * Inversesqrt(Dot(p, p));
	}

	// ������( ����� )�Ķ���
	inline void FillRectPoints(XY centPos, uint8_t directionAngle, float w, float h, xx::List<XY>& outPoints)
	{
		outPoints.Resize(4);
		XY rectXyInc = GetXyInc(directionAngle);
		XY rectYxInc{ rectXyInc.y, -rectXyInc.x };
		outPoints[0] = centPos - (rectYxInc * w + rectXyInc * h) * 0.5f;
		outPoints[1] = outPoints[0] + rectYxInc * w;
		outPoints[2] = outPoints[1] + rectXyInc * h;
		outPoints[3] = outPoints[0] + rectXyInc * h;
	}

	// �������( ����� )�Ķ���
	inline void FillFanPoints(XY pos, uint8_t directionAngle, uint8_t fanAngle, float radius, int segmentCount, xx::List<XY>& outPoints)
	{
		outPoints.Resize(segmentCount + 2);
		outPoints[0] = pos;
		auto beginAngle = (uint8_t)(directionAngle - fanAngle / 2);
		outPoints[1] = pos + (GetXyInc(beginAngle) * radius);
		for (auto i = 1; i <= segmentCount; ++i)
		{
			auto a = (uint8_t)(beginAngle + (float)fanAngle * i / segmentCount);
			outPoints[i + 1] = pos + (GetXyInc(a) * radius);
		}
	}

	// ���ݶ���Ͷ����������ͶӰ
	inline static void FillProjectionAxis(xx::List<XY>& points, xx::List<XY>& projectionAxis)
	{
		auto count = (int)points.dataLen;
		points.Reserve(points.dataLen + 1);
		points[count] = points[0];			// Ӫ��һ���պ���״ ����ѭ���� if
		projectionAxis.Resize(count + 1);
		for (auto i = 0; i < count - 1; i++)
		{
			auto edge = points[i + 1] - points[i];
			projectionAxis[i] = Normalize({ edge.y, -edge.x });
		}
	}

	// �ж�Բ�������Ƿ��ཻ
	inline static bool CirclePolygonIntersect(XY circleCenter, float radius, xx::List<XY> const& points, xx::List<XY>& projectionAxis)
	{
		auto count = (int)points.dataLen;

		auto closedXY = circleCenter - points[0];
		auto closedDis = GetDistancePow2(closedXY);
		for (auto i = 0; i < count; i++)
		{
			// �ҵ�����κ�Բ������ĵ�
			auto deltaXy = circleCenter - points[i];
			auto dis = GetDistancePow2(deltaXy);
			if (dis < closedDis)
			{
				closedXY = deltaXy;
				closedDis = dis;
			}
		}
		projectionAxis[count] = Normalize(closedXY);

		// ��ÿ��ͶӰ����ͶӰ����κ�Բ��,���Ƿ����ཻ
		for (auto j = 0; j <= count; j++)
		{
			auto& axis = projectionAxis[j];
			auto circleMaxProjection = Dot(circleCenter, axis) + radius;
			auto circleMinProjection = circleMaxProjection - 2 * radius;

			auto rectMaxProjection = std::numeric_limits<float>::min();
			auto rectMinProjection = std::numeric_limits<float>::max();

			for (auto i = 0; i < count; i++)
			{
				auto projection = Dot(points[i], axis);
				rectMaxProjection = MAX(rectMaxProjection, projection);
				rectMinProjection = MIN(rectMinProjection, projection);
			}
			if (MIN(circleMaxProjection, rectMaxProjection) <= MAX(circleMinProjection, rectMinProjection))
			{
				return false;
			}
		}

		return true;
	}
};

extern XyMath xyMath;			// ��Ҫ��ĳ�� cpp ��ʵ��
