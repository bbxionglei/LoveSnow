local ls = require "lovesnow"
local core = require "lovesnow.core"
require "lovesnow.manager"	-- import manager apis
local string = string

local services = {}
local command = {}
local instance = {} -- for confirm (function command.LAUNCH / command.ERROR / command.LAUNCHOK)
local launch_session = {} -- for command.QUERY, service_address -> session

local function handle_to_address(handle)
	return tonumber("0x" .. string.sub(handle , 2))
end

local NORET = {}

function command.LIST()
	local list = {}
	for k,v in pairs(services) do
		list[ls.address(k)] = v
	end
	return list
end

function command.STAT()
	local list = {}
	for k,v in pairs(services) do
		local ok, stat = pcall(ls.call,k,"debug","STAT")
		if not ok then
			stat = string.format("ERROR (%s)",v)
		end
		list[ls.address(k)] = stat
	end
	return list
end

function command.KILL(_, handle)
	handle = handle_to_address(handle)
	ls.kill(handle)
	local ret = { [ls.address(handle)] = tostring(services[handle]) }
	services[handle] = nil
	return ret
end

function command.MEM()
	local list = {}
	for k,v in pairs(services) do
		local ok, kb, bytes = pcall(ls.call,k,"debug","MEM")
		if not ok then
			list[ls.address(k)] = string.format("ERROR (%s)",v)
		else
			list[ls.address(k)] = string.format("%.2f Kb (%s)",kb,v)
		end
	end
	return list
end

function command.GC()
	for k,v in pairs(services) do
		ls.send(k,"debug","GC")
	end
	return command.MEM()
end

function command.REMOVE(_, handle, kill)
	services[handle] = nil
	local response = instance[handle]
	if response then
		-- instance is dead
		response(not kill)	-- return nil to caller of newservice, when kill == false
		instance[handle] = nil
		launch_session[handle] = nil
	end

	-- don't return (ls.ret) because the handle may exit
	return NORET
end

local function launch_service(service, ...)
	local param = table.concat({...}, " ")
	local inst = ls.launch(service, param)
	local session = ls.context()
	local response = ls.response()
	if inst then
		services[inst] = service .. " " .. param
		instance[inst] = response
		launch_session[inst] = session
	else
		response(false)
		return
	end
	return inst
end

function command.LAUNCH(_, service, ...)
	launch_service(service, ...)
	return NORET
end

function command.LOGLAUNCH(_, service, ...)
	local inst = launch_service(service, ...)
	if inst then
		core.command("LOGON", ls.address(inst))
	end
	return NORET
end

function command.ERROR(address)
	-- see serivce-src/service_lua.c
	-- init failed
	local response = instance[address]
	if response then
		response(false)
		launch_session[address] = nil
		instance[address] = nil
	end
	services[address] = nil
	return NORET
end

function command.LAUNCHOK(address)
	-- init notice
	local response = instance[address]
	if response then
		response(true, address)
		instance[address] = nil
		launch_session[address] = nil
	end

	return NORET
end

function command.QUERY(_, request_session)
	for address, session in pairs(launch_session) do
		if session == request_session then
			return address
		end
	end
end

-- for historical reasons, launcher support text command (for C service)

ls.register_protocol {
	name = "text",
	id = ls.PTYPE_TEXT,
	unpack = ls.tostring,
	dispatch = function(session, address , cmd)
		if cmd == "" then
			command.LAUNCHOK(address)
		elseif cmd == "ERROR" then
			command.ERROR(address)
		else
			error ("Invalid text command " .. cmd)
		end
	end,
}

ls.dispatch("lua", function(session, address, cmd , ...)
	cmd = string.upper(cmd)
	local f = command[cmd]
	if f then
		local ret = f(address, ...)
		if ret ~= NORET then
			ls.ret(ls.pack(ret))
		end
	else
		ls.ret(ls.pack {"Unknown command"} )
	end
end)

ls.start(function() end)