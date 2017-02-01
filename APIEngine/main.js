var Gdax = require('gdax');
const stream = require('stream');
var UINT64 = require('/usr/local/lib/node_modules/cuint').UINT64;

var fs = require('fs');
var log_file = fs.createWriteStream(__dirname + '/orders.log', {flags : 'w'});
var numOrders= 0;

function crc32(buff) {
  var crc = ~0;
  var lut = [
    0x00000000,0x1DB71064,0x3B6E20C8,0x26D930AC,0x76DC4190,0x6B6B51F4,0x4DB26158,0x5005713C,
    0xEDB88320,0xF00F9344,0xD6D6A3E8,0xCB61B38C,0x9B64C2B0,0x86D3D2D4,0xA00AE278,0xBDBDF21C
  ];

  var length = buff.length;
  var current = 0;
  while (length--) {
    crc = lut[(crc ^  buff[current]      ) & 0x0F] ^ (crc >>> 4);
    crc = lut[(crc ^ (buff[current] >>> 4)) & 0x0F] ^ (crc >>> 4);
    current++;
  }
  return ~crc;
}

String.prototype.padStart = function (len, str) {
    var padLen = len - this.length;
    var a = this;
    for (var i = 0; i < padLen; i++) {
        a = str + a;
    }
    return a.toString();
};

setup();

const headerPacketSize = 6;
const createOrderPacketSize = 36;
const udpMaxSize = 65507;

const maxPacketSize = parseInt(udpMaxSize / createOrderPacketSize) * createOrderPacketSize + headerPacketSize;

var packetBuffer = null;
var position = 0;

function setup() {
    var websocket = new Gdax.WebsocketClient();
    websocket.on('close', setup);
    websocket.on('message', function(data) {
        if (data.type !== "received")
            return;
        // console.log(data);
        if (!data.price || !data.size)
            return;
        if (!packetBuffer || (position + createOrderPacketSize + headerPacketSize >= maxPacketSize)) {
            if (packetBuffer !== null) {
                var checkSumBuff = new Uint8Array(packetBuffer, 6, maxPacketSize - 6);
                var dataviewHead = new DataView(packetBuffer, 0, 6);
                dataviewHead.setUint8(0, 1); // version
                dataviewHead.setUint32(1, crc32(checkSumBuff), true);

                log_file.write(new Buffer(packetBuffer));
            }
            packetBuffer = new ArrayBuffer(maxPacketSize);
            position = headerPacketSize;
        }
        var dataview = new DataView(packetBuffer, position, createOrderPacketSize);
        position += createOrderPacketSize;


        var order_id_data = new Uint8Array(16);
        var order_id = data.order_id.replace('-', '');
        for (var i = 0; i < 16; i++) {
            order_id_data[i] = parseInt(data.order_id.substr(i, 2), 16);
        }
        var price = (UINT64(data.price.replace('.', ''), 10).toString(16)).padStart(16, '0');
        var qty =  (UINT64(data.size.replace('.', ''), 10).toString(16)).padStart(16, '0');

        var side = data.side === "buy" ? 0 : 1;

        // var orderId = parseInt(new Date().getTime());

        // var aProtocol = new Uint8Array(buffer, 0, 1);
        // var aChecksum = new Uint32Array(buffer, 1, 1);
        // var aOrderType = new Uint8Array(buffer, 5, 1);
        // var aOrderId = new Int64(buffer, 6);
        // var aPrice = new Int64(buffer, 14);
        // var aQty = new Int64(buffer, 22);
        // var aNewOrder = new Int64(buffer, 30);

        var orderType = 0;
        switch (data.order_type) {
            case 'limit':
                break;
            case 'market':
                orderType = 1;
                break;
            case 'stop':
                //orderType = 2; // shouldnt get this.
                //break;
            default:
                return; // ignore anything else.
        }

        dataview.setUint8(0, 0); // ActionType (CREATE_ORDER)
        dataview.setUint8(1, 0); // Flags (post_only).
        dataview.setUint8(2, (side << 4) | orderType);
        dataview.setUint8(3, 0); // Unused

        for (var i = 0; i < 16; i++) {
            dataview.setUint8(4 + i, order_id_data[i]);
        }
        for (var i = 0; i < 8; i++) {
            dataview.setUint8(20 + i, parseInt(price.substr((7 - i) * 2, 2) || '0', 16));
        }
        // console.log("Price: ", price);
        for (var i = 0; i < 8; i++) {
            dataview.setUint8(28 + i, parseInt(qty.substr((7 - i) * 2, 2) || '0', 16));
        }
        // console.log("Qty: ", qty);
        //price.toString(14);
        //qty.toString(22);
        //dataview.setUint32(14, price, true);
        //dataview.setUint32(22, qty, true);


        // aOrderId.setValue('0'.repeat(8) + orderId.toString(16));
        // aPrice.setValue('0'.repeat(8) + price.toString(16));
        // aQty.setValue('0'.repeat(8) + qty.toString(16));

        numOrders++;
        process.stdout.write(`\rHave ${numOrders} Orders`);

    });
}
