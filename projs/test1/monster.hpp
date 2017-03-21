MonsterBase::MonsterBase()
{
	auto& ms = *scene().monsters;
	sceneContainerIndex = (uint32_t)ms.dataLen;
	ms.Add(this);
}
MonsterBase::~MonsterBase()
{
	// Create ʱ��� throw exception ���ܵ�������������
	if (sceneContainerIndex < 0) return;

	// �� �������� �����Ƴ�, ͬ���±�
	auto& ms = *scene().monsters;
	auto& buf = ms.buf;
	auto& dataLen = ms.dataLen;
	--dataLen;
	if (dataLen == this->sceneContainerIndex) return;	// last one
	else
	{
		buf[this->sceneContainerIndex] = buf[dataLen];
		buf[dataLen]->sceneContainerIndex = this->sceneContainerIndex;
	}
}

Monster1::Monster1() : MonsterBase()
{
	SetFSM(CreateFSM<FSMLua>(this, "monster1.lua"));
}
Monster2::Monster2() : MonsterBase()
{
	SetFSM(CreateFSM<FSMLua>(this, "monster2.lua"));
}
