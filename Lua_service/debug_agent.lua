local ls = require "lovesnow"
local debugchannel = require "lovesnow.debugchannel"

local CMD = {}

local channel

function CMD.start(address, fd)
	assert(channel == nil, "start more than once")
	ls.error(string.format("Attach to :%08x", address))
	local handle
	channel, handle = debugchannel.create()
	local ok, err = pcall(ls.call, address, "debug", "REMOTEDEBUG", fd, handle)
	if not ok then
		ls.ret(ls.pack(false, "Debugger attach failed"))
	else
		-- todo hook
		ls.ret(ls.pack(true))
	end
	ls.exit()
end

function CMD.cmd(cmdline)
	channel:write(cmdline)
end

function CMD.ping()
	ls.ret()
end

ls.start(function()
	ls.dispatch("lua", function(_,_,cmd,...)
		local f = CMD[cmd]
		f(...)
	end)
end)
