-- ȡ����紫�����������: 1. Ŀ����ʵ��   2. �־û�����
local self, store = ...

-- ��������������ػ�
local yield = coroutine.yield
local scene = _G.scene

print( "scene.lua" )

local m1 = self:CreateMonster("Monster1")
while true do
	print( "ticks = "..self:ticks()..", lua mem = "..collectgarbage("count") )
	if m1:Ensure() then
		print( "m1 is alive" )
	end
	yield()
end


--[[
local m2 = self:CreateMonster("Monster2")
if not m2:Ensure() then
	print( "Monster2 create failed." )
end

while true do
	print( "ticks = "..self:ticks()..", lua mem = "..collectgarbage("count") )
	if m1:Ensure() then
		print( "m1 is alive" )
		m1:x( m1:x() + 1 )
	else
		print( "m1 is released" )
	end

	if m2:Ensure() then
		print( "m2 is alive" )
		m2:x( m2:x() + 1 )
	else
		print( "m2 is released" )
	end

	yield()
end
]]
