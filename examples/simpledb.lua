local ls = require "lovesnow"
require "lovesnow.manager"	-- import ls.register
local db = {}

local command = {}

function command.GET(key)
	return db[key]
end

function command.SET(key, value)
	local last = db[key]
	db[key] = value
	return last
end

ls.start(function()
	ls.dispatch("lua", function(session, address, cmd, ...)
		cmd = cmd:upper()
		if cmd == "PING" then
			assert(session == 0)
			local str = (...)
			if #str > 20 then
				str = str:sub(1,20) .. "...(" .. #str .. ")"
			end
			ls.error(string.format("%s ping %s", ls.address(address), str))
			return
		end
		local f = command[cmd]
		if f then
			ls.ret(ls.pack(f(...)))
		else
			error(string.format("Unknown command %s", tostring(cmd)))
		end
	end)
--	ls.traceproto("lua", false)	-- true off tracelog
	ls.register "SIMPLEDB"
end)
