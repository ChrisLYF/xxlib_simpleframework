struct FSMLua : FSMBase
{
	lua_State* co = nullptr;		// ����Э��ָ��

	xx::MemHeader_MPObject errMH;	// ������������ṩ�ڴ�ͷ
	xx::String err;

	virtual int Update() override;
	template<typename T>
	FSMLua(T* owner, char const* fn);
	~FSMLua();
};
