#include "UnityGeom.h"
#include <cstdint>
#include <iostream>
#include "xx_mempool.h"
#include "../test1/xymath.h"

XyMath xyMath;

int main()
{
	{
		UnityGeom ug;
		ug.Load("1006.bytes");
		std::cout << "1006.bytes loaded." << std::endl;

		UnityVertex uvmin, uvmax;
		ug.GetBound(uvmin, uvmax);
		std::cout << "uvmin = " << uvmin.x << ", " << uvmin.y << ", " << uvmin.z << "";
		std::cout << "uvmax = " << uvmax.x << ", " << uvmax.y << ", " << uvmax.z << std::endl;

		std::vector<UnityVertex> path;
		//b = ug.FindPath(uv1, uv2, path);
		xx::Stopwatch sw;
		int count = 0;
		//float y;
		//b = ug.GetHeight(99, 48, y);
		//b = ug.GetHeight(239, 41, y);
		for (int i = 0; i < 1; ++i)
		{
			//auto b = ug.GetHeight(2.115, -1.399, y);
			path.clear();
			UnityVertex uv1{ 99, 0, 48 };
			auto b = ug.GetHeight(uv1.x, uv1.z, uv1.y);
			if (b) ++count;
			//UnityVertex uv2{ 237, 0, 40 };
			UnityVertex uv2{ 121, 0, 73 };
			b = ug.GetHeight(uv2.x, uv2.z, uv2.y);
			if (b) ++count;
			b = ug.FindPath(uv1, uv2, path);
			if (b) ++count;
		}
		std::cout << count << " " << path.size() << std::endl;
		std::cout << "ms = " << sw() << std::endl;

		// todo: �� path ������ö��һ����, ֱ��ö�ٵ�ͷ




		int offset = 1;													// �������( ������Ҫ�������������е�״ֵ̬ )

		XY pos = { path[0].x, path[0].z };								// ģ���ȡ�ֵĵ�ǰλ��( ��ѯ���ص�·�����׸���Ӧ�þ���·����ѯ��ʼʱ����� )
		uint8_t angle;													// ģ��ֵĵ�ǰ�Ƕ�

		float moveDistance = 0.5f;										// �����ƶ�����( ģ������ö�ȡ, ÿ���� / ÿ��֡ = ÿ֡�ƶ����� ��֡�ƶ����� )
		while (moveDistance > 0  && offset < path.size())				// ԭ���ǲ��ϵ�����һ���ƶ�, ֱ����������û�е�Ϊֹ
		{
			XY tar = { path[offset].x, path[offset].z };				// ȡ�ƶ�Ŀ���
			auto dir = tar - pos;										// �㷽������
			angle = xyMath.GetAngleUnsafe(dir);							// ���õ� ���� �Ƕ�
			auto disPow2 = xyMath.GetDistancePow2(dir);					// �㵱ǰ������Ŀ���ľ����ƽ��
			if (moveDistance * moveDistance >= disPow2)					// ���Ŀ��㲻���������ƶ�����, �򽫵�ǰ���������õ�, ����ȥ�Ѿ��ƶ����ľ��볤, �����õ�
			{
				pos = tar;
				moveDistance -= sqrtf(disPow2);
				++offset;
			}
			else														// ���Ŀ��������ƶ�����, ���� Ŀ��� �ƶ� ��ǰ�� ���볤��
			{
				//pos = pos + xyMath.Normalize(dir) * moveDistance;		// ʸ����һ���� * ������Ϊƫ����
				pos = pos + xyMath.GetXyInc(angle) * moveDistance;		// �����ȡƫ����( �ȿ������쵫���ܲ��Ƿǳ���ȷ )
				moveDistance = 0.0f;
			}
		}

		if (moveDistance != 0.0f)										// ̽��ѭ������ԭ��: ��ɨ����, ����ʣһ�ξ���, �Ǿ�����Ŀ������ƶ���ξ���
		{
			//pos = pos + xyMath.GetXyInc(tar->pos - pos) * moveDistance;
		}

		// todo: �� pos �� angle Ӧ�õ���


	}
	return 0;
}
