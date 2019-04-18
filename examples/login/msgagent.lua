local ls = require "lovesnow"

ls.register_protocol {
	name = "client",
	id = ls.PTYPE_CLIENT,
	unpack = ls.tostring,
}

local gate
local userid, subid

local CMD = {}

function CMD.login(source, uid, sid, secret)
	-- you may use secret to make a encrypted data stream
	ls.error(string.format("%s is login", uid))
	gate = source
	userid = uid
	subid = sid
	-- you may load user data from database
end

local function logout()
	if gate then
		ls.call(gate, "lua", "logout", userid, subid)
	end
	ls.exit()
end

function CMD.logout(source)
	-- NOTICE: The logout MAY be reentry
	ls.error(string.format("%s is logout", userid))
	logout()
end

function CMD.afk(source)
	-- the connection is broken, but the user may back
	ls.error(string.format("AFK"))
end

ls.start(function()
	-- If you want to fork a work thread , you MUST do it in CMD.login
	ls.dispatch("lua", function(session, source, command, ...)
		local f = assert(CMD[command])
		ls.ret(ls.pack(f(source, ...)))
	end)

	ls.dispatch("client", function(_,_, msg)
		-- the simple echo service
		ls.sleep(10)	-- sleep a while
		ls.ret(msg)
	end)
end)
