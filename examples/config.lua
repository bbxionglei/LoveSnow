include "../examples/config.path.lua"

-- preload = "./examples/preload.lua"	-- run preload.lua before every lua service run
thread = 1
logger = nil
logpath = "."
harbor = 0
-- address = "127.0.0.1:2526"
-- master = "127.0.0.1:2013"
start = "main"	-- main script
bootstrap = "ls_load_script bootstrap"	-- The service for bootstrap
-- standalone = "0.0.0.0:2013"
-- snax_interface_g = "snax_g"
--cpath = root.."cservice/?.so"
cpath = root.."Bin/?"
daemon = "ls.pid"
