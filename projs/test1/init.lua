print( "init.lua" )
local yield = coroutine.yield

-- ���ֻ�ʺ� AI, û�г־û�״̬��¼
function Sleep( ticks, cond )
	for i = 1, ticks do
		if cond() then return end
		yield()
	end
end

-- todo: more utils
