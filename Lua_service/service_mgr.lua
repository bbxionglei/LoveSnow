local ls = require "lovesnow"
require "lovesnow.manager"	-- import ls.register
local snax = require "lovesnow.snax"

local cmd = {}
local service = {}

local function request(name, func, ...)
	local ok, handle = pcall(func, ...)
	local s = service[name]
	assert(type(s) == "table")
	if ok then
		service[name] = handle
	else
		service[name] = tostring(handle)
	end

	for _,v in ipairs(s) do
		ls.wakeup(v.co)
	end

	if ok then
		return handle
	else
		error(tostring(handle))
	end
end

local function waitfor(name , func, ...)
	local s = service[name]
	if type(s) == "number" then
		return s
	end
	local co = coroutine.running()

	if s == nil then
		s = {}
		service[name] = s
	elseif type(s) == "string" then
		error(s)
	end

	assert(type(s) == "table")

	local session, source = ls.context()

	if s.launch == nil and func then
		s.launch = {
			session = session,
			source = source,
			co = co,
		}
		return request(name, func, ...)
	end

	table.insert(s, {
		co = co,
		session = session,
		source = source,
	})
	ls.wait()
	s = service[name]
	if type(s) == "string" then
		error(s)
	end
	assert(type(s) == "number")
	return s
end

local function read_name(service_name)
	if string.byte(service_name) == 64 then -- '@'
		return string.sub(service_name , 2)
	else
		return service_name
	end
end

function cmd.LAUNCH(service_name, subname, ...)
	local realname = read_name(service_name)

	if realname == "snaxd" then
		return waitfor(service_name.."."..subname, snax.rawnewservice, subname, ...)
	else
		return waitfor(service_name, ls.newservice, realname, subname, ...)
	end
end

function cmd.QUERY(service_name, subname)
	local realname = read_name(service_name)

	if realname == "snaxd" then
		return waitfor(service_name.."."..subname)
	else
		return waitfor(service_name)
	end
end

local function list_service()
	local result = {}
	for k,v in pairs(service) do
		if type(v) == "string" then
			v = "Error: " .. v
		elseif type(v) == "table" then
			local querying = {}
			if v.launch then
				local session = ls.task(v.launch.co)
				local launching_address = ls.call(".launcher", "lua", "QUERY", session)
				if launching_address then
					table.insert(querying, "Init as " .. ls.address(launching_address))
					table.insert(querying,  ls.call(launching_address, "debug", "TASK", "init"))
					table.insert(querying, "Launching from " .. ls.address(v.launch.source))
					table.insert(querying, ls.call(v.launch.source, "debug", "TASK", v.launch.session))
				end
			end
			if #v > 0 then
				table.insert(querying , "Querying:" )
				for _, detail in ipairs(v) do
					table.insert(querying, ls.address(detail.source) .. " " .. tostring(ls.call(detail.source, "debug", "TASK", detail.session)))
				end
			end
			v = table.concat(querying, "\n")
		else
			v = ls.address(v)
		end

		result[k] = v
	end

	return result
end


local function register_global()
	function cmd.GLAUNCH(name, ...)
		local global_name = "@" .. name
		return cmd.LAUNCH(global_name, ...)
	end

	function cmd.GQUERY(name, ...)
		local global_name = "@" .. name
		return cmd.QUERY(global_name, ...)
	end

	local mgr = {}

	function cmd.REPORT(m)
		mgr[m] = true
	end

	local function add_list(all, m)
		local harbor = "@" .. ls.harbor(m)
		local result = ls.call(m, "lua", "LIST")
		for k,v in pairs(result) do
			all[k .. harbor] = v
		end
	end

	function cmd.LIST()
		local result = {}
		for k in pairs(mgr) do
			pcall(add_list, result, k)
		end
		local l = list_service()
		for k, v in pairs(l) do
			result[k] = v
		end
		return result
	end
end

local function register_local()
	local function waitfor_remote(cmd, name, ...)
		local global_name = "@" .. name
		local local_name
		if name == "snaxd" then
			local_name = global_name .. "." .. (...)
		else
			local_name = global_name
		end
		return waitfor(local_name, ls.call, "SERVICE", "lua", cmd, global_name, ...)
	end

	function cmd.GLAUNCH(...)
		return waitfor_remote("LAUNCH", ...)
	end

	function cmd.GQUERY(...)
		return waitfor_remote("QUERY", ...)
	end

	function cmd.LIST()
		return list_service()
	end

	ls.call("SERVICE", "lua", "REPORT", ls.self())
end

ls.start(function()
	ls.dispatch("lua", function(session, address, command, ...)
		local f = cmd[command]
		if f == nil then
			ls.ret(ls.pack(nil, "Invalid command " .. command))
			return
		end

		local ok, r = pcall(f, ...)

		if ok then
			ls.ret(ls.pack(r))
		else
			ls.ret(ls.pack(nil, r))
		end
	end)
	local handle = ls.localname ".service"
	if  handle then
		ls.error(".service is already register by ", ls.address(handle))
		ls.exit()
	else
		ls.register(".service")
	end
	if ls.getenv "standalone" then
		ls.register("SERVICE")
		register_global()
	else
		register_local()
	end
end)
