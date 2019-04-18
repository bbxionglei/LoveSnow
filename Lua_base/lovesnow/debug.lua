local table = table
local extern_dbgcmd = {}

local function init(ls, export)
	local internal_info_func

	function ls.info_func(func)
		internal_info_func = func
	end

	local dbgcmd

	local function init_dbgcmd()
		dbgcmd = {}

		function dbgcmd.MEM()
			local kb, bytes = collectgarbage "count"
			ls.ret(ls.pack(kb,bytes))
		end

		function dbgcmd.GC()

			collectgarbage "collect"
		end

		function dbgcmd.STAT()
			local stat = {}
			stat.task = ls.task()
			stat.mqlen = ls.stat "mqlen"
			stat.cpu = ls.stat "cpu"
			stat.message = ls.stat "message"
			ls.ret(ls.pack(stat))
		end

		function dbgcmd.TASK(session)
			if session then
				ls.ret(ls.pack(ls.task(session)))
			else
				local task = {}
				ls.task(task)
				ls.ret(ls.pack(task))
			end
		end

		function dbgcmd.INFO(...)
			if internal_info_func then
				ls.ret(ls.pack(internal_info_func(...)))
			else
				ls.ret(ls.pack(nil))
			end
		end

		function dbgcmd.EXIT()
			ls.exit()
		end

		function dbgcmd.RUN(source, filename, ...)
			local inject = require "lovesnow.inject"
			local args = table.pack(...)
			local ok, output = inject(ls, source, filename, args, export.dispatch, ls.register_protocol)
			collectgarbage "collect"
			ls.ret(ls.pack(ok, table.concat(output, "\n")))
		end

		function dbgcmd.TERM(service)
			ls.term(service)
		end

		function dbgcmd.REMOTEDEBUG(...)
			local remotedebug = require "lovesnow.remotedebug"
			remotedebug.start(export, ...)
		end

		function dbgcmd.SUPPORT(pname)
			return ls.ret(ls.pack(ls.dispatch(pname) ~= nil))
		end

		function dbgcmd.PING()
			return ls.ret()
		end

		function dbgcmd.LINK()
			ls.response()	-- get response , but not return. raise error when exit
		end

		function dbgcmd.TRACELOG(proto, flag)
			if type(proto) ~= "string" then
				flag = proto
				proto = "lua"
			end
			ls.error(string.format("Turn trace log %s for %s", flag, proto))
			ls.traceproto(proto, flag)
			ls.ret()
		end

		return dbgcmd
	end -- function init_dbgcmd

	local function _debug_dispatch(session, address, cmd, ...)
		dbgcmd = dbgcmd or init_dbgcmd() -- lazy init dbgcmd
		local f = dbgcmd[cmd] or extern_dbgcmd[cmd]
		assert(f, cmd)
		f(...)
	end

	ls.register_protocol {
		name = "debug",
		id = assert(ls.PTYPE_DEBUG),
		pack = assert(ls.pack),
		unpack = assert(ls.unpack),
		dispatch = _debug_dispatch,
	}
end

local function reg_debugcmd(name, fn)
	extern_dbgcmd[name] = fn
end

return {
	init = init,
	reg_debugcmd = reg_debugcmd,
}
