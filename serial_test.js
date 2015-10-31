/**
 * Created by missionhealth on 15/10/30.
 */

// 树莓派只有一个串口,默认被用来做console了,需要先禁用
var SERIAL_PORT = '/dev/ttyAMA0';
// G3的数据包长度为24字节
var PACKAGE_LEN = 24;

// serial
var SerialPort = require("serialport").SerialPort;
// RPI PWM
var wpi = require('wiring-pi');

var serialPort = new SerialPort(SERIAL_PORT, {
    baudrate: 9600
});

wpi.setup('wpi');

var pin = 1;
wpi.pinMode(pin, wpi.PWM_OUTPUT);
wpi.pwmWrite(pin, 0);

serialPort.on("open", function () {
    console.log(SERIAL_PORT + ' open success');

    // 处理完整的package
    var handle_package = function(package) {
        console.log('#####################');
        console.log(package);
        // data length should be 24bytes
        if (package.length !== 24) {
            console.log('data package length[24, %d]', package.length);
            return;
        }

        // check data package length, should be 20
        var package_length = package[2] * 256 + package[3];
        if (package_length !== 20) {
            console.log('RECV data package length error[20, %d]', package_length);
            return;
        }

        // check CRC
        var crc = 0;
        for (var i = 0; i < package.length - 2; i++) {
            crc += package[i];
        }
        crc = crc % (256*256);
        var package_crc = package[22] * 256 + package[23];
        if (package_crc !== crc) {
            console.log('data package crc error[%d, %d]', package_crc, crc);
            return;
        }

        // all is OK, let's get real value
        var index = 4;
        if (package[0] === 0x42 && package[1] === 0x4d) {
            // PM1.0(CF=1)
            var pm1_0 = package[index++] * 256 + package[index++];
            // PM2.5(CF=1)
            var pm2_5 = package[index++] * 256 + package[index++];
            // PM10(CF=1)
            var pm10 = package[index++] * 256 + package[index++];

            console.log('(CF=1) -> [%d, %d, %d]', pm1_0, pm2_5, pm10);

            // PM1.0(大气环境下)
            var pm_air_1_0 = package[index++] * 256 + package[index++];
            // PM2.5(大气环境下)
            var pm_air_2_5 = package[index++] * 256 + package[index++];
            // PM10(大气环境下)
            var pm_air_10 = package[index++] * 256 + package[index++];

            console.log('大气环境 -> [%d, %d, %d]', pm_air_1_0, pm_air_2_5, pm_air_10);

            // 数据7,8,9保留
        } else {
            console.log('RECV data err: ');
            console.log(package);
        }
    };

    var whole_package = new Buffer(PACKAGE_LEN);
    var package_index = 0;
    serialPort.on('data', function(data) {
        // test
        console.log(data);

        for (var i = 0; i < data.length; i++) {
            // check package header
            if (package_index === 0) {
                if (data[i] === 0x42 && data[i + 1] === 0x4d) {
                    whole_package[package_index++] = data[i];
                }
            } else if (package_index < PACKAGE_LEN){
                whole_package[package_index++] = data[i];
            }

            if (package_index === PACKAGE_LEN) {
                handle_package(whole_package);
                package_index = 0;
            }
        }
    });
});

serialPort.on('error', function(err) {
    console.log('Open serial port error: ' + err);
});


