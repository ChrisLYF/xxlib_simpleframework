#include "UnityGeom.h"
#include <cstdint>
#include <iostream>

// �Ƕ�תΪ 360 ������ϵ ���� 2 λ���ȵ�����
int ConvertToXZAngleForSend(uint8_t a)
{
	return (int)(float((uint8_t)(-(int8_t)a + 64)) / 256.0f * 36000.0f);
}

// �� 360 �����ö���Ƕ�ֵ
uint8_t ConvertFromXZ360Angle(int a)
{
	return (uint8_t)(-(int8_t)(uint8_t)(a / 360.0f * 256.0f) + 64);
}

int main()
{
	std::cout << ConvertToXZAngleForSend(128) << std::endl;
	std::cout << (int)ConvertFromXZ360Angle(180) << std::endl;


	return 0;
	UnityGeom ug;
	ug.Load("1000.bytes");
	float y;
	auto b = ug.GetHeight(10.8f, 24.8f, y);
	return 0;
}
