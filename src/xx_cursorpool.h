#pragma once
namespace xx
{
	// ѭ�����ó�. ͨ�����ڹ��������� list / buf ֮���, ���п��ܲ���������øú���
	template<typename T, size_t Size>
	struct CursorPool
	{
		static_assert(Size > 0 && !(Size & (Size - 1)));	// 2^n ensure
		std::array<T, Size> pool;
		size_t cursor = 0;
		// ÿִ��һ��, �� cursor Ϊ�±귵�� pool �ж��������. cursor Ϊѭ������
		T& GetOne()
		{
			auto& rtv = pool[cursor];
			cursor = (cursor + 1) & (Size - 1);
			return rtv;
		}
	};

	/*
	// ʾ��:

	MP mp;
	CursorPool<xx::List<int>*, 4> p;
	for (auto& o : p.pool)
	{
		o = mp.CreateWithoutTypeId<xx::List<int>>();
	}
	auto list = p.GetOne()
	auto list = p.GetOne()
	auto list = p.GetOne()

	for (auto& o : fp.pool)
	{
		o->Release();
	}

	
	CursorPool<std::vector<xxxx>, 8> p;

	auto& vec = p.GetOne()
	auto& vec = p.GetOne()
	auto& vec = p.GetOne()

	*/

}
