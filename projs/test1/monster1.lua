local self = ...
local x = self:x()				-- bak
Sleep( 10, function() 
	return self:x() - x > 3;	-- x �нϴ�仯�� sleep ��ֹ
end )
