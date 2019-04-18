local ls = require "lovesnow"
local snax   = require "lovesnow.snax"
local socket = require "lovesnow.socket"

local function split_cmdline(cmdline)
	local split = {}
	for i in string.gmatch(cmdline, "%S+") do
		table.insert(split,i)
	end
	return split
end

local function console_main_loop()
	local stdin = socket.stdin()
	while true do
		local cmdline = socket.readline(stdin, "\n")
		print("console_main_loop:")
		print(cmdline)
		local split = split_cmdline(cmdline)
		local command = split[1]
		if command == "snax" then
			pcall(snax.newservice, select(2, table.unpack(split)))
		elseif cmdline ~= "" then
			pcall(ls.newservice, cmdline)
		end
	end
end

ls.start(function()
	ls.fork(console_main_loop)
end)
