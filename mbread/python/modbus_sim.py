

import logging
from pymodbus.datastore import ModbusSequentialDataBlock, ModbusServerContext, ModbusSlaveContext
from pymodbus.server import StartTcpServer
HOST = "127.0.0.1"
PORT = 1502
VALUES = [100, 200, 300, 400, 500, 600, 700, 800, 900, 1000]
logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
store = ModbusSlaveContext(
hr=ModbusSequentialDataBlock(0, VALUES),
zero_mode=True,
)
context = ModbusServerContext(slaves=store, single=True)
print(f"Modbus TCP simulator on {HOST}:{PORT}", flush=True)
print(f"Holding registers 0-{len(VALUES) - 1}: {VALUES}", flush=True)
StartTcpServer(context=context, address=(HOST, PORT))
