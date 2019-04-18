local ls = require "lovesnow"
local codecache = require "lovesnow.codecache"
local core = require "lovesnow.core"
local socket = require "lovesnow.socket"
local snax = require "lovesnow.snax"
local memory = require "lovesnow.memory"
local httpd = require "http.httpd"
local sockethelper = require "http.sockethelper"

local arg = table.pack(...)
assert(arg.n <= 2)
local ip = (arg.n == 2 and arg[1] or "127.0.0.1")
local port = tonumber(arg[arg.n])

local COMMAND = {}
local COMMANDX = {}

local function format_table(t)
	local index = {}
	for k in pairs(t) do
		table.insert(index, k)
	end
	table.sort(index, function(a, b) return tostring(a) < tostring(b) end)
	local result = {}
	for _,v in ipairs(index) do
		table.insert(result, string.format("%s:%s",v,tostring(t[v])))
	end
	return table.concat(result,"\t")
end

local function dump_line(print, key, value)
	if type(value) == "table" then
		print(key, format_table(value))
	else
		print(key,tostring(value))
	end
end

local function dump_list(print, list)
	local index = {}
	for k in pairs(list) do
		table.insert(index, k)
	end
	table.sort(index, function(a, b) return tostring(a) < tostring(b) end)
	for _,v in ipairs(index) do
		dump_line(print, v, list[v])
	end
end

local function split_cmdline(cmdline)
	local split = {}
	for i in string.gmatch(cmdline, "%S+") do
		table.insert(split,i)
	end
	return split
end

local function docmd(cmdline, print, fd)
	local split = split_cmdline(cmdline)
	local command = split[1]
	local cmd = COMMAND[command]
	local ok, list
	if cmd then
		ok, list = pcall(cmd, table.unpack(split,2))
	else
		cmd = COMMANDX[command]
		if cmd then
			split.fd = fd
			split[1] = cmdline
			ok, list = pcall(cmd, split)
		else
			print("Invalid command, type help for command list")
		end
	end

	if ok then
		if list then
			if type(list) == "string" then
				print(list)
			else
				dump_list(print, list)
			end
		end
		print("<CMD OK>")
	else
		print(list)
		print("<CMD Error>")
	end
end

local function console_main_loop(stdin, print)
	print("Welcome to ls console")
	ls.error(stdin, "connected")
	local ok, err = pcall(function()
		while true do
			local cmdline = socket.readline(stdin, "\n")
			if not cmdline then
				break
			end
			if cmdline:sub(1,4) == "GET " then
				-- http
				local code, url = httpd.read_request(sockethelper.readfunc(stdin, cmdline.. "\n"), 8192)
				local cmdline = url:sub(2):gsub("/"," ")
				docmd(cmdline, print, stdin)
				break
			end
			if cmdline ~= "" then
				docmd(cmdline, print, stdin)
			end
		end
	end)
	if not ok then
		ls.error(stdin, err)
	end
	ls.error(stdin, "disconnected")
	socket.close(stdin)
end

ls.start(function()
	local listen_socket = socket.listen (ip, port)
	ls.error("Start debug console at " .. ip .. ":" .. port)
	socket.start(listen_socket , function(id, addr)
		local function print(...)
			local t = { ... }
			for k,v in ipairs(t) do
				t[k] = tostring(v)
			end
			socket.write(id, table.concat(t,"\t"))
			socket.write(id, "\n")
		end
		socket.start(id)
		ls.fork(console_main_loop, id , print)
	end)
end)

function COMMAND.help()
	return {
		help = "This help message",
		list = "List all the service",
		stat = "Dump all stats",
		info = "info address : get service infomation",
		exit = "exit address : kill a lua service",
		kill = "kill address : kill service",
		mem = "mem : show memory status",
		gc = "gc : force every lua service do garbage collect",
		start = "lanuch a new lua service",
		snax = "lanuch a new snax service",
		clearcache = "clear lua code cache",
		service = "List unique service",
		task = "task address : show service task detail",
		inject = "inject address luascript.lua",
		logon = "logon address",
		logoff = "logoff address",
		log = "launch a new lua service with log",
		debug = "debug address : debug a lua service",
		signal = "signal address sig",
		cmem = "Show C memory info",
		shrtbl = "Show shared short string table info",
		ping = "ping address",
		call = "call address ...",
		trace = "trace address [proto] [on|off]",
		netstat = "netstat : show netstat",
	}
end

function COMMAND.clearcache()
	codecache.clear()
end

function COMMAND.start(...)
	local ok, addr = pcall(ls.newservice, ...)
	if ok then
		if addr then
			return { [ls.address(addr)] = ... }
		else
			return "Exit"
		end
	else
		return "Failed"
	end
end

function COMMAND.log(...)
	local ok, addr = pcall(ls.call, ".launcher", "lua", "LOGLAUNCH", "ls_load_script", ...)
	if ok then
		if addr then
			return { [ls.address(addr)] = ... }
		else
			return "Failed"
		end
	else
		return "Failed"
	end
end

function COMMAND.snax(...)
	local ok, s = pcall(snax.newservice, ...)
	if ok then
		local addr = s.handle
		return { [ls.address(addr)] = ... }
	else
		return "Failed"
	end
end

function COMMAND.service()
	return ls.call("SERVICE", "lua", "LIST")
end

local function adjust_address(address)
	local prefix = address:sub(1,1)
	if prefix == '.' then
		return assert(ls.localname(address), "Not a valid name")
	elseif prefix ~= ':' then
		address = assert(tonumber("0x" .. address), "Need an address") | (ls.harbor(ls.self()) << 24)
	end
	return address
end

function COMMAND.list()
	return ls.call(".launcher", "lua", "LIST")
end

function COMMAND.stat()
	return ls.call(".launcher", "lua", "STAT")
end

function COMMAND.mem()
	return ls.call(".launcher", "lua", "MEM")
end

function COMMAND.kill(address)
	return ls.call(".launcher", "lua", "KILL", address)
end

function COMMAND.gc()
	return ls.call(".launcher", "lua", "GC")
end

function COMMAND.exit(address)
	ls.send(adjust_address(address), "debug", "EXIT")
end

function COMMAND.inject(address, filename, ...)
	address = adjust_address(address)
	local f = io.open(filename, "rb")
	if not f then
		return "Can't open " .. filename
	end
	local source = f:read "*a"
	f:close()
	local ok, output = ls.call(address, "debug", "RUN", source, filename, ...)
	if ok == false then
		error(output)
	end
	return output
end

function COMMAND.task(address)
	address = adjust_address(address)
	return ls.call(address,"debug","TASK")
end

function COMMAND.info(address, ...)
	address = adjust_address(address)
	return ls.call(address,"debug","INFO", ...)
end

function COMMANDX.debug(cmd)
	local address = adjust_address(cmd[2])
	local agent = ls.newservice "debug_agent"
	local stop
	local term_co = coroutine.running()
	local function forward_cmd()
		repeat
			-- notice :  It's a bad practice to call socket.readline from two threads (this one and console_main_loop), be careful.
			ls.call(agent, "lua", "ping")	-- detect agent alive, if agent exit, raise error
			local cmdline = socket.readline(cmd.fd, "\n")
			cmdline = cmdline and cmdline:gsub("(.*)\r$", "%1")
			if not cmdline then
				ls.send(agent, "lua", "cmd", "cont")
				break
			end
			ls.send(agent, "lua", "cmd", cmdline)
		until stop or cmdline == "cont"
	end
	ls.fork(function()
		pcall(forward_cmd)
		if not stop then	-- block at ls.call "start"
			term_co = nil
		else
			ls.wakeup(term_co)
		end
	end)
	local ok, err = ls.call(agent, "lua", "start", address, cmd.fd)
	stop = true
	if term_co then
		-- wait for fork coroutine exit.
		ls.wait(term_co)
	end

	if not ok then
		error(err)
	end
end

function COMMAND.logon(address)
	address = adjust_address(address)
	core.command("LOGON", ls.address(address))
end

function COMMAND.logoff(address)
	address = adjust_address(address)
	core.command("LOGOFF", ls.address(address))
end

function COMMAND.signal(address, sig)
	address = ls.address(adjust_address(address))
	if sig then
		core.command("SIGNAL", string.format("%s %d",address,sig))
	else
		core.command("SIGNAL", address)
	end
end

function COMMAND.cmem()
	local info = memory.info()
	local tmp = {}
	for k,v in pairs(info) do
		tmp[ls.address(k)] = v
	end
	tmp.total = memory.total()
	tmp.block = memory.block()

	return tmp
end

function COMMAND.shrtbl()
	local n, total, longest, space = memory.ssinfo()
	return { n = n, total = total, longest = longest, space = space }
end

function COMMAND.ping(address)
	address = adjust_address(address)
	local ti = ls.now()
	ls.call(address, "debug", "PING")
	ti = ls.now() - ti
	return tostring(ti)
end

local function toboolean(x)
	return x and (x == "true" or x == "on")
end

function COMMAND.trace(address, proto, flag)
	address = adjust_address(address)
	if flag == nil then
		if proto == "on" or proto == "off" then
			proto = toboolean(proto)
		end
	else
		flag = toboolean(flag)
	end
	ls.call(address, "debug", "TRACELOG", proto, flag)
end

function COMMANDX.call(cmd)
	local address = adjust_address(cmd[2])
	local cmdline = assert(cmd[1]:match("%S+%s+%S+%s(.+)") , "need arguments")
	local args_func = assert(load("return " .. cmdline, "debug console", "t", {}), "Invalid arguments")
	local args = table.pack(pcall(args_func))
	if not args[1] then
		error(args[2])
	end
	local rets = table.pack(ls.call(address, "lua", table.unpack(args, 2, args.n)))
	return rets
end

local function bytes(size)
	if size == nil or size == 0 then
		return
	end
	if size < 1024 then
		return size
	end
	if size < 1024 * 1024 then
		return tostring(size/1024) .. "K"
	end
	return tostring(size/(1024*1024)) .. "M"
end

local function convert_stat(info)
	local now = ls.now()
	local function time(t)
		if t == nil then
			return
		end
		t = now - t
		if t < 6000 then
			return tostring(t/100) .. "s"
		end
		local hour = t // (100*60*60)
		t = t - hour * 100 * 60 * 60
		local min = t // (100*60)
		t = t - min * 100 * 60
		local sec = t / 100
		return string.format("%s%d:%.2gs",hour == 0 and "" or (hour .. ":"),min,sec)
	end

	info.address = ls.address(info.address)
	info.read = bytes(info.read)
	info.write = bytes(info.write)
	info.wbuffer = bytes(info.wbuffer)
	info.rtime = time(info.rtime)
	info.wtime = time(info.wtime)
end

function COMMAND.netstat()
	local stat = socket.netstat()
	for _, info in ipairs(stat) do
		convert_stat(info)
	end
	return stat
end