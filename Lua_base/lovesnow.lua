local c = require "lovesnow.core"
local tostring = tostring
local coroutine = coroutine
local assert = assert
local pairs = pairs
local pcall = pcall
local table = table

local profile = require "lovesnow.profile"

local cresume = profile.resume
local running_thread = nil
local init_thread = nil

local function coroutine_resume(co, ...)
	running_thread = co
	return cresume(co, ...)
end
local coroutine_yield = profile.yield
local coroutine_create = coroutine.create

local proto = {}
local ls = {
	-- read ls.h
	PTYPE_TEXT = 0,
	PTYPE_RESPONSE = 1,
	PTYPE_MULTICAST = 2,
	PTYPE_CLIENT = 3,
	PTYPE_SYSTEM = 4,
	PTYPE_HARBOR = 5,
	PTYPE_SOCKET = 6,
	PTYPE_ERROR = 7,
	PTYPE_QUEUE = 8,	-- used in deprecated mqueue, use ls.queue instead
	PTYPE_DEBUG = 9,
	PTYPE_LUA = 10,
	PTYPE_SNAX = 11,
	PTYPE_TRACE = 12,	-- use for debug trace
}

-- code cache
ls.cache = require "lovesnow.codecache"

function ls.register_protocol(class)
	local name = class.name
	local id = class.id
	assert(proto[name] == nil and proto[id] == nil)
	assert(type(name) == "string" and type(id) == "number" and id >=0 and id <=255)
	proto[name] = class
	proto[id] = class
end

local session_id_coroutine = {}
local session_coroutine_id = {}
local session_coroutine_address = {}
local session_coroutine_tracetag = {}
local unresponse = {}

local wakeup_queue = {}
local sleep_session = {}

local watching_session = {}
local error_queue = {}
local fork_queue = {}

-- suspend is function
local suspend


----- monitor exit

local function dispatch_error_queue()
	local session = table.remove(error_queue,1)
	if session then
		local co = session_id_coroutine[session]
		session_id_coroutine[session] = nil
		return suspend(co, coroutine_resume(co, false))
	end
end

local function _error_dispatch(error_session, error_source)
	ls.ignoreret()	-- don't return for error
	if error_session == 0 then
		-- error_source is down, clear unreponse set
		for resp, address in pairs(unresponse) do
			if error_source == address then
				unresponse[resp] = nil
			end
		end
		for session, srv in pairs(watching_session) do
			if srv == error_source then
				table.insert(error_queue, session)
			end
		end
	else
		-- capture an error for error_session
		if watching_session[error_session] then
			table.insert(error_queue, error_session)
		end
	end
end

-- coroutine reuse

local coroutine_pool = setmetatable({}, { __mode = "kv" })

local function co_create(f)
	local co = table.remove(coroutine_pool)
	if co == nil then
		co = coroutine_create(function(...)
			f(...)
			while true do
				local session = session_coroutine_id[co]
				if session and session ~= 0 then
					local source = debug.getinfo(f,"S")
					ls.error(string.format("Maybe forgot response session %s from %s : %s:%d",
						session,
						ls.address(session_coroutine_address[co]),
						source.source, source.linedefined))
				end
				-- coroutine exit
				local tag = session_coroutine_tracetag[co]
				if tag ~= nil then
					if tag then c.trace(tag, "end")	end
					session_coroutine_tracetag[co] = nil
				end
				local address = session_coroutine_address[co]
				if address then
					session_coroutine_id[co] = nil
					session_coroutine_address[co] = nil
				end

				-- recycle co into pool
				f = nil
				coroutine_pool[#coroutine_pool+1] = co
				-- recv new main function f
				f = coroutine_yield "SUSPEND"
				f(coroutine_yield())
			end
		end)
	else
		-- pass the main function f to coroutine, and restore running thread
		local running = running_thread
		coroutine_resume(co, f)
		running_thread = running
	end
	return co
end

local function dispatch_wakeup()
	local token = table.remove(wakeup_queue,1)
	if token then
		local session = sleep_session[token]
		if session then
			local co = session_id_coroutine[session]
			local tag = session_coroutine_tracetag[co]
			if tag then c.trace(tag, "resume") end
			session_id_coroutine[session] = "BREAK"
			return suspend(co, coroutine_resume(co, false, "BREAK"))
		end
	end
end

-- suspend is local function
function suspend(co, result, command)
	if not result then
		local session = session_coroutine_id[co]
		if session then -- coroutine may fork by others (session is nil)
			local addr = session_coroutine_address[co]
			if session ~= 0 then
				-- only call response error
				local tag = session_coroutine_tracetag[co]
				if tag then c.trace(tag, "error") end
				c.send(addr, ls.PTYPE_ERROR, session, "")
			end
			session_coroutine_id[co] = nil
			session_coroutine_address[co] = nil
			session_coroutine_tracetag[co] = nil
		end
		ls.fork(function() end)	-- trigger command "SUSPEND"
		error(debug.traceback(co,tostring(command)))
	end
	if command == "SUSPEND" then
		dispatch_wakeup()
		dispatch_error_queue()
	elseif command == "QUIT" then
		-- service exit
		return
	elseif command == "USER" then
		-- See ls.coutine for detail
		error("Call ls.coroutine.yield out of ls.coroutine.resume\n" .. debug.traceback(co))
	elseif command == nil then
		-- debug trace
		return
	else
		error("Unknown command : " .. command .. "\n" .. debug.traceback(co))
	end
end

function ls.timeout(ti, func)
	local session = c.intcommand("TIMEOUT",ti)
	assert(session)
	local co = co_create(func)
	assert(session_id_coroutine[session] == nil)
	session_id_coroutine[session] = co
	return co	-- for debug
end

local function suspend_sleep(session, token)
	local tag = session_coroutine_tracetag[running_thread]
	if tag then c.trace(tag, "sleep", 2) end
	session_id_coroutine[session] = running_thread
	assert(sleep_session[token] == nil, "token duplicative")
	sleep_session[token] = session

	return coroutine_yield "SUSPEND"
end

function ls.sleep(ti, token)
	local session = c.intcommand("TIMEOUT",ti)
	assert(session)
	token = token or coroutine.running()
	local succ, ret = suspend_sleep(session, token)
	sleep_session[token] = nil
	if succ then
		return
	end
	if ret == "BREAK" then
		return "BREAK"
	else
		error(ret)
	end
end

function ls.yield()
	return ls.sleep(0)
end

function ls.wait(token)
	local session = c.genid()
	token = token or coroutine.running()
	local ret, msg = suspend_sleep(session, token)
	sleep_session[token] = nil
	session_id_coroutine[session] = nil
end

function ls.self()
	return c.addresscommand "REG"
end

function ls.localname(name)
	return c.addresscommand("QUERY", name)
end

ls.now = c.now
ls.hpc = c.hpc	-- high performance counter

local traceid = 0
function ls.trace(info)
	ls.error("TRACE", session_coroutine_tracetag[running_thread])
	if session_coroutine_tracetag[running_thread] == false then
		-- force off trace log
		return
	end
	traceid = traceid + 1

	local tag = string.format(":%08x-%d",ls.self(), traceid)
	session_coroutine_tracetag[running_thread] = tag
	if info then
		c.trace(tag, "trace " .. info)
	else
		c.trace(tag, "trace")
	end
end

function ls.tracetag()
	return session_coroutine_tracetag[running_thread]
end

local starttime

function ls.starttime()
	if not starttime then
		starttime = c.intcommand("STARTTIME")
	end
	return starttime
end

function ls.time()
	return ls.now()/100 + (starttime or ls.starttime())
end

function ls.exit()
	fork_queue = {}	-- no fork coroutine can be execute after ls.exit
	ls.send(".launcher","lua","REMOVE",ls.self(), false)
	-- report the sources that call me
	for co, session in pairs(session_coroutine_id) do
		local address = session_coroutine_address[co]
		if session~=0 and address then
			c.send(address, ls.PTYPE_ERROR, session, "")
		end
	end
	for resp in pairs(unresponse) do
		resp(false)
	end
	-- report the sources I call but haven't return
	local tmp = {}
	for session, address in pairs(watching_session) do
		tmp[address] = true
	end
	for address in pairs(tmp) do
		c.send(address, ls.PTYPE_ERROR, 0, "")
	end
	c.command("EXIT")
	-- quit service
	coroutine_yield "QUIT"
end

function ls.getenv(key)
	return (c.command("GETENV",key))
end

function ls.setenv(key, value)
	assert(c.command("GETENV",key) == nil, "Can't setenv exist key : " .. key)
	c.command("SETENV",key .. " " ..value)
end

function ls.send(addr, typename, ...)
	local p = proto[typename]
	return c.send(addr, p.id, 0 , p.pack(...))
end

function ls.rawsend(addr, typename, msg, sz)
	local p = proto[typename]
	return c.send(addr, p.id, 0 , msg, sz)
end

ls.genid = assert(c.genid)

ls.redirect = function(dest,source,typename,...)
	return c.redirect(dest, source, proto[typename].id, ...)
end

ls.pack = assert(c.pack)
ls.packstring = assert(c.packstring)
ls.unpack = assert(c.unpack)
ls.tostring = assert(c.tostring)
ls.trash = assert(c.trash)

local function yield_call(service, session)
	watching_session[session] = service
	session_id_coroutine[session] = running_thread
	local succ, msg, sz = coroutine_yield "SUSPEND"
	watching_session[session] = nil
	if not succ then
		error "call failed"
	end
	return msg,sz
end

-- ls.call(".launcher", "lua" , "LAUNCH", "ls_load_script", name, ...)
function ls.call(addr, typename, ...)
	local tag = session_coroutine_tracetag[running_thread]
	if tag then
		c.trace(tag, "call", 2)
		c.send(addr, ls.PTYPE_TRACE, 0, tag)
	end

	local p = proto[typename]
	local session = c.send(addr, p.id , nil , p.pack(...))
	if session == nil then
		error("call to invalid address " .. ls.address(addr))
	end
	return p.unpack(yield_call(addr, session))
end

function ls.rawcall(addr, typename, msg, sz)
	local tag = session_coroutine_tracetag[running_thread]
	if tag then
		c.trace(tag, "call", 2)
		c.send(addr, ls.PTYPE_TRACE, 0, tag)
	end
	local p = proto[typename]
	local session = assert(c.send(addr, p.id , nil , msg, sz), "call to invalid address")
	return yield_call(addr, session)
end

function ls.tracecall(tag, addr, typename, msg, sz)
	c.trace(tag, "tracecall begin")
	c.send(addr, ls.PTYPE_TRACE, 0, tag)
	local p = proto[typename]
	local session = assert(c.send(addr, p.id , nil , msg, sz), "call to invalid address")
	local msg, sz = yield_call(addr, session)
	c.trace(tag, "tracecall end")
	return msg, sz
end

function ls.ret(msg, sz)
	msg = msg or ""
	local tag = session_coroutine_tracetag[running_thread]
	if tag then c.trace(tag, "response") end
	local co_session = session_coroutine_id[running_thread]
	session_coroutine_id[running_thread] = nil
	if co_session == 0 then
		if sz ~= nil then
			c.trash(msg, sz)
		end
		return false	-- send don't need ret
	end
	local co_address = session_coroutine_address[running_thread]
	if not co_session then
		error "No session"
	end
	local ret = c.send(co_address, ls.PTYPE_RESPONSE, co_session, msg, sz)
	if ret then
		return true
	elseif ret == false then
		-- If the package is too large, returns false. so we should report error back
		c.send(co_address, ls.PTYPE_ERROR, co_session, "")
	end
	return false
end

function ls.context()
	local co_session = session_coroutine_id[running_thread]
	local co_address = session_coroutine_address[running_thread]
	return co_session, co_address
end

function ls.ignoreret()
	-- We use session for other uses
	session_coroutine_id[running_thread] = nil
end

function ls.response(pack)
	pack = pack or ls.pack

	local co_session = assert(session_coroutine_id[running_thread], "no session")
	session_coroutine_id[running_thread] = nil
	local co_address = session_coroutine_address[running_thread]
	if co_session == 0 then
		--  do not response when session == 0 (send)
		return function() end
	end
	local function response(ok, ...)
		if ok == "TEST" then
			return unresponse[response] ~= nil
		end
		if not pack then
			error "Can't response more than once"
		end

		local ret
		if unresponse[response] then
			if ok then
				ret = c.send(co_address, ls.PTYPE_RESPONSE, co_session, pack(...))
				if ret == false then
					-- If the package is too large, returns false. so we should report error back
					c.send(co_address, ls.PTYPE_ERROR, co_session, "")
				end
			else
				ret = c.send(co_address, ls.PTYPE_ERROR, co_session, "")
			end
			unresponse[response] = nil
			ret = ret ~= nil
		else
			ret = false
		end
		pack = nil
		return ret
	end
	unresponse[response] = co_address

	return response
end

function ls.retpack(...)
	return ls.ret(ls.pack(...))
end

function ls.wakeup(token)
	if sleep_session[token] then
		table.insert(wakeup_queue, token)
		return true
	end
end

function ls.dispatch(typename, func)
	local p = proto[typename]
	if func then
		local ret = p.dispatch
		p.dispatch = func
		return ret
	else
		return p and p.dispatch
	end
end

local function unknown_request(session, address, msg, sz, prototype)
	ls.error(string.format("Unknown request (%s): %s", prototype, c.tostring(msg,sz)))
	error(string.format("Unknown session : %d from %x", session, address))
end

function ls.dispatch_unknown_request(unknown)
	local prev = unknown_request
	unknown_request = unknown
	return prev
end

local function unknown_response(session, address, msg, sz)
	ls.error(string.format("Response message : %s" , c.tostring(msg,sz)))
	error(string.format("Unknown session : %d from %x", session, address))
end

function ls.dispatch_unknown_response(unknown)
	local prev = unknown_response
	unknown_response = unknown
	return prev
end

function ls.fork(func,...)
	local n = select("#", ...)
	local co
	if n == 0 then
		co = co_create(func)
	else
		local args = { ... }
		co = co_create(function() func(table.unpack(args,1,n)) end)
	end
	table.insert(fork_queue, co)
	return co
end

local trace_source = {}

local function raw_dispatch_message(prototype, msg, sz, session, source)
	-- ls.PTYPE_RESPONSE = 1, read ls.h
	if prototype == 1 then
		local co = session_id_coroutine[session]
		if co == "BREAK" then
			session_id_coroutine[session] = nil
		elseif co == nil then
			unknown_response(session, source, msg, sz)
		else
			local tag = session_coroutine_tracetag[co]
			if tag then c.trace(tag, "resume") end
			session_id_coroutine[session] = nil
			suspend(co, coroutine_resume(co, true, msg, sz))
		end
	else
		local p = proto[prototype]
		if p == nil then
			if prototype == ls.PTYPE_TRACE then
				-- trace next request
				trace_source[source] = c.tostring(msg,sz)
			elseif session ~= 0 then
				c.send(source, ls.PTYPE_ERROR, session, "")
			else
				unknown_request(session, source, msg, sz, prototype)
			end
			return
		end

		local f = p.dispatch
		if f then
			local co = co_create(f)
			session_coroutine_id[co] = session
			session_coroutine_address[co] = source
			local traceflag = p.trace
			if traceflag == false then
				-- force off
				trace_source[source] = nil
				session_coroutine_tracetag[co] = false
			else
				local tag = trace_source[source]
				if tag then
					trace_source[source] = nil
					c.trace(tag, "request")
					session_coroutine_tracetag[co] = tag
				elseif traceflag then
					-- set running_thread for trace
					running_thread = co
					ls.trace()
				end
			end
			suspend(co, coroutine_resume(co, session,source, p.unpack(msg,sz)))
		else
			trace_source[source] = nil
			if session ~= 0 then
				c.send(source, ls.PTYPE_ERROR, session, "")
			else
				unknown_request(session, source, msg, sz, proto[prototype].name)
			end
		end
	end
end

function ls.dispatch_message(...)
	local succ, err = pcall(raw_dispatch_message,...)  -- 处理消息
	-- 处理其他 ls.fork 出来的协程
	while true do
		local key,co = next(fork_queue)
		if co == nil then
			break
		end
		fork_queue[key] = nil
		local fork_succ, fork_err = pcall(suspend,co,coroutine_resume(co))
		if not fork_succ then
			if succ then
				succ = false
				err = tostring(fork_err)
			else
				err = tostring(err) .. "\n" .. tostring(fork_err)
			end
		end
	end
	assert(succ, tostring(err))
end

function ls.newservice(name, ...)
	return ls.call(".launcher", "lua" , "LAUNCH", "ls_load_script", name, ...)
end

function ls.uniqueservice(global, ...)
	if global == true then
		return assert(ls.call(".service", "lua", "GLAUNCH", ...))
	else
		return assert(ls.call(".service", "lua", "LAUNCH", global, ...))
	end
end

function ls.queryservice(global, ...)
	if global == true then
		return assert(ls.call(".service", "lua", "GQUERY", ...))
	else
		return assert(ls.call(".service", "lua", "QUERY", global, ...))
	end
end

function ls.address(addr)
	if type(addr) == "number" then
		return string.format(":%08x",addr)
	else
		return tostring(addr)
	end
end

function ls.harbor(addr)
	return c.harbor(addr)
end

ls.error = c.error
ls.tracelog = c.trace

-- true: force on
-- false: force off
-- nil: optional (use ls.trace() to trace one message)
function ls.traceproto(prototype, flag)
	local p = assert(proto[prototype])
	p.trace = flag
end

----- register protocol
do
	local REG = ls.register_protocol

	REG {
		name = "lua",
		id = ls.PTYPE_LUA,
		pack = ls.pack,
		unpack = ls.unpack,
	}

	REG {
		name = "response",
		id = ls.PTYPE_RESPONSE,
	}

	REG {
		name = "error",
		id = ls.PTYPE_ERROR,
		unpack = function(...) return ... end,
		dispatch = _error_dispatch,
	}
end

local init_func = {}

function ls.init(f, name)
	assert(type(f) == "function")
	if init_func == nil then
		f()
	else
		table.insert(init_func, f)
		if name then
			assert(type(name) == "string")
			assert(init_func[name] == nil)
			init_func[name] = f
		end
	end
end

local function init_all()
	local funcs = init_func
	init_func = nil
	if funcs then
		for _,f in ipairs(funcs) do
			f()
		end
	end
end

local function ret(f, ...)
	f()
	return ...
end

local function init_template(start, ...)
	init_all()
	init_func = {}
	return ret(init_all, start(...))
end

function ls.pcall(start, ...)
	return xpcall(init_template, debug.traceback, start, ...)
end

function ls.init_service(start)
	local ok, err = ls.pcall(start)
	if not ok then
		ls.error("init service failed: " .. tostring(err))
		ls.send(".launcher","lua", "ERROR")
		ls.exit()
	else
		ls.send(".launcher","lua", "LAUNCHOK")
	end
end

function ls.start(start_func)
	c.callback(ls.dispatch_message) -- 设置回调函数
	init_thread = ls.timeout(0, function()
		ls.init_service(start_func)
		init_thread = nil
	end)
end

function ls.endless()
	return (c.intcommand("STAT", "endless") == 1)
end

function ls.mqlen()
	return c.intcommand("STAT", "mqlen")
end

function ls.stat(what)
	return c.intcommand("STAT", what)
end

function ls.task(ret)
	if ret == nil then
		local t = 0
		for session,co in pairs(session_id_coroutine) do
			t = t + 1
		end
		return t
	end
	if ret == "init" then
		if init_thread then
			return debug.traceback(init_thread)
		else
			return
		end
	end
	local tt = type(ret)
	if tt == "table" then
		for session,co in pairs(session_id_coroutine) do
			ret[session] = debug.traceback(co)
		end
		return
	elseif tt == "number" then
		local co = session_id_coroutine[ret]
		if co then
			return debug.traceback(co)
		else
			return "No session"
		end
	elseif tt == "thread" then
		for session, co in pairs(session_id_coroutine) do
			if co == ret then
				return session
			end
		end
		return
	end
end

function ls.term(service)
	return _error_dispatch(0, service)
end

function ls.memlimit(bytes)
	debug.getregistry().memlimit = bytes
	ls.memlimit = nil	-- set only once
end

-- Inject internal debug framework
--local debug = require "lovesnow.debug"
--debug.init(ls, {
--	dispatch = ls.dispatch_message,
--	suspend = suspend,
--})

return ls
