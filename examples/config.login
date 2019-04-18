thread = 8
logger = nil
harbor = 0
start = "main"
bootstrap = "ls_load_script bootstrap"	-- The service for bootstrap
luaservice = "./service/?.lua;./examples/login/?.lua"
lualoader = "lualib/loader.lua"
cpath = "./cservice/?.so"
