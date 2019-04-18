local ls = require "lovesnow"
local socket = require "lovesnow.socket"
local socketdriver = require "lovesnow.socketdriver"
require "lovesnow.manager"	-- import ls.launch, ...
local table = table

local slaves = {}
local connect_queue = {}
local globalname = {}
local queryname = {}
local harbor = {}
local harbor_service
local monitor = {}
local monitor_master_set = {}

local function read_package(fd)
	local sz = socket.read(fd, 1)
	assert(sz, "closed")
	sz = string.byte(sz)
	local content = assert(socket.read(fd, sz), "closed")
	return ls.unpack(content)
end

local function pack_package(...)
	local message = ls.packstring(...)
	local size = #message
	assert(size <= 255 , "too long")
	return string.char(size) .. message
end

local function monitor_clear(id)
	local v = monitor[id]
	if v then
		monitor[id] = nil
		for _, v in ipairs(v) do
			v(true)
		end
	end
end

local function connect_slave(slave_id, address)
	local ok, err = pcall(function()
		if slaves[slave_id] == nil then
			local fd = assert(socket.open(address), "Can't connect to "..address)
			socketdriver.nodelay(fd)
			ls.error(string.format("Connect to harbor %d (fd=%d), %s", slave_id, fd, address))
			slaves[slave_id] = fd
			monitor_clear(slave_id)
			socket.abandon(fd)
			ls.send(harbor_service, "harbor", string.format("S %d %d",fd,slave_id))
		end
	end)
	if not ok then
		ls.error(err)
	end
end

local function ready()
	local queue = connect_queue
	connect_queue = nil
	for k,v in pairs(queue) do
		connect_slave(k,v)
	end
	for name,address in pairs(globalname) do
		ls.redirect(harbor_service, address, "harbor", 0, "N " .. name)
	end
end

local function response_name(name)
	local address = globalname[name]
	if queryname[name] then
		local tmp = queryname[name]
		queryname[name] = nil
		for _,resp in ipairs(tmp) do
			resp(true, address)
		end
	end
end

local function monitor_master(master_fd)
	while true do
		local ok, t, id_name, address = pcall(read_package,master_fd)
		if ok then
			if t == 'C' then
				if connect_queue then
					connect_queue[id_name] = address
				else
					connect_slave(id_name, address)
				end
			elseif t == 'N' then
				globalname[id_name] = address
				response_name(id_name)
				if connect_queue == nil then
					ls.redirect(harbor_service, address, "harbor", 0, "N " .. id_name)
				end
			elseif t == 'D' then
				local fd = slaves[id_name]
				slaves[id_name] = false
				if fd then
					monitor_clear(id_name)
					socket.close(fd)
				end
			end
		else
			ls.error("Master disconnect")
			for _, v in ipairs(monitor_master_set) do
				v(true)
			end
			socket.close(master_fd)
			break
		end
	end
end

local function accept_slave(fd)
	socket.start(fd)
	local id = socket.read(fd, 1)
	if not id then
		ls.error(string.format("Connection (fd =%d) closed", fd))
		socket.close(fd)
		return
	end
	id = string.byte(id)
	if slaves[id] ~= nil then
		ls.error(string.format("Slave %d exist (fd =%d)", id, fd))
		socket.close(fd)
		return
	end
	slaves[id] = fd
	monitor_clear(id)
	socket.abandon(fd)
	ls.error(string.format("Harbor %d connected (fd = %d)", id, fd))
	ls.send(harbor_service, "harbor", string.format("A %d %d", fd, id))
end

ls.register_protocol {
	name = "harbor",
	id = ls.PTYPE_HARBOR,
	pack = function(...) return ... end,
	unpack = ls.tostring,
}

ls.register_protocol {
	name = "text",
	id = ls.PTYPE_TEXT,
	pack = function(...) return ... end,
	unpack = ls.tostring,
}

local function monitor_harbor(master_fd)
	return function(session, source, command)
		local t = string.sub(command, 1, 1)
		local arg = string.sub(command, 3)
		if t == 'Q' then
			-- query name
			if globalname[arg] then
				ls.redirect(harbor_service, globalname[arg], "harbor", 0, "N " .. arg)
			else
				socket.write(master_fd, pack_package("Q", arg))
			end
		elseif t == 'D' then
			-- harbor down
			local id = tonumber(arg)
			if slaves[id] then
				monitor_clear(id)
			end
			slaves[id] = false
		else
			ls.error("Unknown command ", command)
		end
	end
end

function harbor.REGISTER(fd, name, handle)
	assert(globalname[name] == nil)
	globalname[name] = handle
	response_name(name)
	socket.write(fd, pack_package("R", name, handle))
	ls.redirect(harbor_service, handle, "harbor", 0, "N " .. name)
end

function harbor.LINK(fd, id)
	if slaves[id] then
		if monitor[id] == nil then
			monitor[id] = {}
		end
		table.insert(monitor[id], ls.response())
	else
		ls.ret()
	end
end

function harbor.LINKMASTER()
	table.insert(monitor_master_set, ls.response())
end

function harbor.CONNECT(fd, id)
	if not slaves[id] then
		if monitor[id] == nil then
			monitor[id] = {}
		end
		table.insert(monitor[id], ls.response())
	else
		ls.ret()
	end
end

function harbor.QUERYNAME(fd, name)
	if name:byte() == 46 then	-- "." , local name
		ls.ret(ls.pack(ls.localname(name)))
		return
	end
	local result = globalname[name]
	if result then
		ls.ret(ls.pack(result))
		return
	end
	local queue = queryname[name]
	if queue == nil then
		socket.write(fd, pack_package("Q", name))
		queue = { ls.response() }
		queryname[name] = queue
	else
		table.insert(queue, ls.response())
	end
end

ls.start(function()
	local master_addr = ls.getenv "master"
	local harbor_id = tonumber(ls.getenv "harbor")
	local slave_address = assert(ls.getenv "address")
	local slave_fd = socket.listen(slave_address)
	ls.error("slave connect to master " .. tostring(master_addr))
	local master_fd = assert(socket.open(master_addr), "Can't connect to master")

	ls.dispatch("lua", function (_,_,command,...)
		local f = assert(harbor[command])
		f(master_fd, ...)
	end)
	ls.dispatch("text", monitor_harbor(master_fd))

	harbor_service = assert(ls.launch("ls_harborm", harbor_id, ls.self()))

	local hs_message = pack_package("H", harbor_id, slave_address)
	socket.write(master_fd, hs_message)
	local t, n = read_package(master_fd)
	assert(t == "W" and type(n) == "number", "slave shakehand failed")
	ls.error(string.format("Waiting for %d harbors", n))
	ls.fork(monitor_master, master_fd)
	if n > 0 then
		local co = coroutine.running()
		socket.start(slave_fd, function(fd, addr)
			ls.error(string.format("New connection (fd = %d, %s)",fd, addr))
			socketdriver.nodelay(fd)
			if pcall(accept_slave,fd) then
				local s = 0
				for k,v in pairs(slaves) do
					s = s + 1
				end
				if s >= n then
					ls.wakeup(co)
				end
			end
		end)
		ls.wait()
		socket.close(slave_fd)
	else
		-- slave_fd does not start, so use close_fd.
		socket.close_fd(slave_fd)
	end
	ls.error("Shakehand ready")
	ls.fork(ready)
end)
