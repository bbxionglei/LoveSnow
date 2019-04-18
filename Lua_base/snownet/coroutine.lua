-- You should use this module (ls.coroutine) instead of origin lua coroutine in ls framework

local coroutine = coroutine
-- origin lua coroutine module
local coroutine_resume = coroutine.resume
local coroutine_yield = coroutine.yield
local coroutine_status = coroutine.status
local coroutine_running = coroutine.running

local select = select
local lsco = {}

lsco.isyieldable = coroutine.isyieldable
lsco.running = coroutine.running
lsco.status = coroutine.status

local ls_coroutines = setmetatable({}, { __mode = "kv" })

function lsco.create(f)
	local co = coroutine.create(f)
	-- mark co as a ls coroutine
	ls_coroutines[co] = true
	return co
end

do -- begin lsco.resume

	local profile = require "lovesnow.profile"
	-- ls use profile.resume_co/yield_co instead of coroutine.resume/yield

	local ls_resume = profile.resume_co
	local ls_yield = profile.yield_co

	local function unlock(co, ...)
		ls_coroutines[co] = true
		return ...
	end

	local function ls_yielding(co, from, ...)
		ls_coroutines[co] = false
		return unlock(co, ls_resume(co, from, ls_yield(from, ...)))
	end

	local function resume(co, from, ok, ...)
		if not ok then
			return ok, ...
		elseif coroutine_status(co) == "dead" then
			-- the main function exit
			ls_coroutines[co] = nil
			return true, ...
		elseif (...) == "USER" then
			return true, select(2, ...)
		else
			-- blocked in ls framework, so raise the yielding message
			return resume(co, from, ls_yielding(co, from, ...))
		end
	end

	-- record the root of coroutine caller (It should be a ls thread)
	local coroutine_caller = setmetatable({} , { __mode = "kv" })

function lsco.resume(co, ...)
	local co_status = ls_coroutines[co]
	if not co_status then
		if co_status == false then
			-- is running
			return false, "cannot resume a ls coroutine suspend by ls framework"
		end
		if coroutine_status(co) == "dead" then
			-- always return false, "cannot resume dead coroutine"
			return coroutine_resume(co, ...)
		else
			return false, "cannot resume none ls coroutine"
		end
	end
	local from = coroutine_running()
	local caller = coroutine_caller[from] or from
	coroutine_caller[co] = caller
	return resume(co, caller, coroutine_resume(co, ...))
end

function lsco.thread(co)
	co = co or coroutine_running()
	if ls_coroutines[co] ~= nil then
		return coroutine_caller[co] , false
	else
		return co, true
	end
end

end -- end of lsco.resume

function lsco.status(co)
	local status = coroutine.status(co)
	if status == "suspended" then
		if ls_coroutines[co] == false then
			return "blocked"
		else
			return "suspended"
		end
	else
		return status
	end
end

function lsco.yield(...)
	return coroutine_yield("USER", ...)
end

do -- begin lsco.wrap

	local function wrap_co(ok, ...)
		if ok then
			return ...
		else
			error(...)
		end
	end

function lsco.wrap(f)
	local co = lsco.create(function(...)
		return f(...)
	end)
	return function(...)
		return wrap_co(lsco.resume(co, ...))
	end
end

end	-- end of lsco.wrap

return lsco
