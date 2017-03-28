SkillBase::SkillBase(MonsterBase* owner)
	: owner(owner)
{
	owner->skills->AddDirect(this);
}

// ��� gcd, cd, target, ����
// ���㵱ǰ������ "������", ��Ҳ�� focus & distance �ж�( ����AI )
bool SkillBase::Avaliable()
{
	return owner->skillsGcd <= scene().ticks
		&& this->cd <= scene().ticks
		&& owner->target
		&& cfg_distance.IntersectPow2(owner->DistancePow2(owner->target.pointer));
}

void SkillBase::Cast()
{
	// ����cd, gcd
	cd = scene().ticks + cfg_cd;
	owner->skillsGcd = scene().ticks + this->cfg_gcd;

	// ��Ѫ & ֪ͨ
	auto& tarHP = owner->target->hp;
	if (tarHP > 0)
	{
		owner->target->Hurt(owner);
		tarHP -= cfg_damage;
		if (tarHP <= 0)
		{
			assert(scene().deadMonsters->Find(owner->target.pointer) == -1);
			scene().deadMonsters->Add(owner->target.pointer);
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
	this->cfg_castStunTimespan = 5;
}

SkillFar::SkillFar(MonsterBase* owner) : SkillBase(owner)
{
	// ģ����������
	this->cfg_distance = { 15, 20 };
	this->cfg_damage = 10;
	this->cfg_cd = 6;
	this->cfg_castStunTimespan = 0;
}
