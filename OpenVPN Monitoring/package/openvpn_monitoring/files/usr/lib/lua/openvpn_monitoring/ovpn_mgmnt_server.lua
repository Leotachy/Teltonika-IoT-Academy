local nixio = require("nixio")
local utility = require("openvpn_monitoring.utility")
local Log = require("openvpn_monitoring.log")

local server_context = {}
local sock

server_context.init = function(ip, port)
    local s, err = nixio.socket("inet", "stream")
    if s == nil then
        Log.error("Failed creating socket: " .. err)
        return
    end

    s:setblocking(true)
    local status, err = s:connect(ip, port)
    if status == nil then
        Log.error("Failed connecting to server: " .. err)
    end

    sock = s
end

server_context.receive = function(match_term)
    local buffer = ""
    while true do
        local data, err = sock:recv(1024)
        if data == nil then
            Log.error("Failed receiving data from server:" .. err)
            break
        end
        if data == "" then
            Log.error("Connection terminated?")
            break
        end

        buffer = buffer .. data
        if buffer:find(match_term, 1, false) then
            break
        end
    end
    return buffer
end

server_context.get_status = function()
    sock:send("status\n")
    local response = server_context.receive("\nEND\r?\n")
    return response
end

server_context.disconnect_client = function(client_name)
    sock:send("kill " .. client_name .. "\n")
    local response = server_context.receive("\n%u+:")
    return response
end

server_context.deinit = function()
    sock:close()
end

return server_context
