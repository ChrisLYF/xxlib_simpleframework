struct FSMLua : FSMBase
{
	lua_State* co = nullptr;		// ����Э��ָ��
	xx::String* err;

	virtual int Update() override;
	lua_State* GetL() const;		// ���ڶ�λ�� owner-> scene ��״̬��
	FSMLua(char const* luacode);
	~FSMLua();
};
