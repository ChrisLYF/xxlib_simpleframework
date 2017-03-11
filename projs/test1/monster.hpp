MonsterBase::~MonsterBase()
{
	if (sceneContainerIndex < 0) return;	// Create ʱ��� throw exception ���ܵ�������������

	// �� �������� �����Ƴ�, ͬ���±�
	auto& buf = this->scene().monsters->buf;
	auto& dataLen = this->scene().monsters->dataLen;
	--dataLen;
	if (dataLen == this->sceneContainerIndex) return;	// last one
	else
	{
		buf[this->sceneContainerIndex] = buf[dataLen];
		buf[dataLen]->sceneContainerIndex = this->sceneContainerIndex;
	}
}

Monster1::Monster1(char const* luacode)
{
	sceneContainerIndex = (uint32_t)this->scene().monsters->dataLen;
	this->scene().monsters->Add(this);
	SetFSM(CreateFSM<FSMLua>(this, luacode));
}
Monster2::Monster2()
{
	sceneContainerIndex = (uint32_t)this->scene().monsters->dataLen;
	this->scene().monsters->Add(this);
}

int Monster2::Update()
{
	return 0;
}
