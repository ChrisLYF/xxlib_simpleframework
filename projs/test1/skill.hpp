SkillBase::SkillBase(MonsterBase* owner)
{
	owner->skills->Add(this);
}

// ��� gcd, cd, target, ����
// ���㵱ǰ������ "������", ��Ҳ�� focus & distance �ж�( ����AI )
bool SkillBase::Avaliable()
{
	return owner->skillsGcd <= scene().ticks
		&& this->cd <= scene().ticks
		&& owner->target
		&& cfg_distance.Test(owner->Distance(owner->target.pointer));
}

void SkillBase::Cast()
{
	// ����cd, gcd
	cd = scene().ticks + cfg_cd;
	owner->skillsGcd = scene().ticks + this->cfg_gcd;

	// ��Ѫ
	auto& tarHP = owner->target->hp;
	if (tarHP)
	{
		tarHP -= cfg_damage;
		if (tarHP <= 0)
		{
			// todo: ��ɱ����? cout 
		}
	}
}

SkillNear::SkillNear(MonsterBase* owner) : SkillBase(owner)
{
	// ģ����������
	this->cfg_distance = { 0, 3 };
	this->cfg_damage = 5;
	this->cfg_cd = 3;
}

SkillFar::SkillFar(MonsterBase* owner) : SkillBase(owner)
{
	// ģ����������
	this->cfg_distance = { 15, 20 };
	this->cfg_damage = 10;
	this->cfg_cd = 6;
}
