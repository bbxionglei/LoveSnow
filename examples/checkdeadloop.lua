local ls = require "lovesnow"

local list = {}

local function timeout_check(ti)
	if not next(list) then
		return
	end
	ls.sleep(ti)	-- sleep 10 sec
	for k,v in pairs(list) do
		ls.error("timout",ti,k,v)
	end
end

ls.start(function()
	ls.error("ping all")
	local list_ret = ls.call(".launcher", "lua", "LIST")
	for addr, desc in pairs(list_ret) do
		list[addr] = desc
		ls.fork(function()
			ls.call(addr,"debug","INFO")
			list[addr] = nil
		end)
	end
	ls.sleep(0)
	timeout_check(100)
	timeout_check(400)
	timeout_check(500)
	ls.exit()
end)
