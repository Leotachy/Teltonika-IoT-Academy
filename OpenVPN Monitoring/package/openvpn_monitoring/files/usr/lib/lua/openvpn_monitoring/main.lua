-- Lukas Andrijauskas
-- Teltonika IoT akademija
-- Nov 9, 2026
-- Simple lua program to manage OpenVPN clients

local ubus = require("ubus")
local uloop = require("uloop")

local Log = require("openvpn_monitoring.log")
local utility = require("openvpn_monitoring.utility")

function get_server_data()

    local uci = require("uci")
    local uci_cursor = uci.cursor()

    local servers = {}

    uci_cursor:foreach("openvpn", "openvpn", function(s)

        -- Is server section
        if (s.mode ~= "server") then
            return
        end

        -- Is enabled
        if (s.enable == "0") then
            return
        end

        -- Get management server address
        local management_server_ip = nil
        local management_server_port = nil
        local extras = uci_cursor:get("openvpn", s[".name"], "extra")
        for i, e in pairs(extras) do
            --			print(e)
            local split = utility.splitstring(e, " ")
            if (#split == 3) then
                if (split[1] == "management") then
                    management_server_ip = split[2]
                    management_server_port = split[3]
                    break
                end
            end
        end

        if (management_server_ip == nil or management_server_port == nil) then
            Log.error("Management server IP or port not found")
        end

        table.insert(servers, {
            internal_name = s[".name"],
            display_name = s["name"],
            management_server_ip = management_server_ip,
            management_server_port = management_server_port
        })
    end)

    return servers
end

function get_client_data(server)

    local ovpn_management_server_context = require("openvpn_monitoring.ovpn_mgmnt_server")
    ovpn_management_server_context.init(server.management_server_ip, server.management_server_port)

    local response = ovpn_management_server_context.get_status()
    -- Parse client data
    local cl_data = {}
    local lines = utility.splitstring(response, "\r?\n")
    local reading_client_data = false
    for i, l in pairs(lines) do
        -- print("Line " .. i .. ": " .. l)
        if (l == "Common Name,Real Address,Bytes Received,Bytes Sent,Connected Since") then
            reading_client_data = true
        elseif (reading_client_data) then
            -- print("LFG")
            local client_data = utility.splitstring(l, ",")
            if (#client_data ~= 5) then
                reading_client_data = false
                break
            end

            table.insert(cl_data, {
                name = client_data[1],
                real_ip_address = client_data[2],
                rx = client_data[3],
                tx = client_data[4],
                connected_since = client_data[5]
            })
        end
    end

    ovpn_management_server_context.deinit()

    return cl_data
end

function disconnect_client(server, client_name)
    local ovpn_management_server_context = require("openvpn_monitoring.ovpn_mgmnt_server")
    ovpn_management_server_context.init(server.management_server_ip, server.management_server_port)

    local response = ovpn_management_server_context.disconnect_client(client_name)

    ovpn_management_server_context.deinit()

    -- Last message in the response is the actual response...
    response = utility.splitstring(response, "\r?\n")
    response = response[#response]
    return response
end

local server_data = get_server_data()

uloop.init()
local ubus_context = ubus.connect()
if (ubus_context == nil) then
    Log.error("Failed to connect to UBUS")
end

-- Create UBUS object for each server
for i, server in pairs(server_data) do

    local methods = {
    	display_name = {function(req, msg)
    	
    	ubus_context:reply(req, {
                display_name = server.display_name
            })
            return 0
    	end, {}},
        list_clients = {function(req, msg)

            local client_data_table = get_client_data(server)

            ubus_context:reply(req, {
                clients = client_data_table
            })

            return 0
        end, {}},
        kick_client = {function(req, msg)

            local client_name = msg.client_name;
            local response = disconnect_client(server, client_name)

            ubus_context:reply(req, {
                message = response
            })

            return 0

        end, {
            client_name = ubus.STRING
        }}
    }

    ubus_context:add({
        ["openvpn." .. server.internal_name] = methods
    })
end

uloop.run()
