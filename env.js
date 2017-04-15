(function() {
  'use strict';

  class Env {

    /**
     * customize your project here to reflect the uuid of your service and characteristics.
     */
    constructor() {
        this.deviceName = 'env_sensor';
        this.serviceUUIDs = [
          "00001234-0000-1000-8000-00805f9b34fb",
          '00001234-0000-1000-8000-00805f9b34fd',
          '00001234-0000-1000-8000-00805f9b3501'
        ],
        this.dustServiceUUID = "00001234-0000-1000-8000-00805f9b34fb";
        this.device = null;
        this.server = null;
        // The cache allows us to hold on to characeristics for access in response to user commands
        this._characteristics = new Map();
    }

    connect(){
        return navigator.bluetooth.requestDevice({
         acceptAllDevices: true,
         optionalServices: [this.serviceUUIDs[0], this.serviceUUIDs[1], this.serviceUUIDs[2] ]
        })
        .then(device => {
            this.device = device;
            return device.gatt.connect();
        })
        .then(server => {
            this.server = server;
            return server.getPrimaryServices();
        })
        .then(services => {
          let queue = Promise.resolve();
          services.forEach(service => {
            queue = queue.then(_ => service.getCharacteristics().then(characteristics => {
              characteristics.forEach(characteristic => {
                characteristic.startNotifications().then(_ => {
                  console.log(characteristic.uuid);
                  characteristic.addEventListener('characteristicvaluechanged', this.handleCharChange);
                });
        });
      }));
    });
    return queue;
  })
  .catch(error => {
    console.log(error);
  })

    }

  handleCharChange(event){
    var elementId = event.target.uuid.substr(-2);
    if ((elementId === 'fe')||(elementId === 'ff')){
      document.getElementById(elementId).innerHTML = event.target.value.getUint32(0, true);
      console.log("int change: " + event.target.value.getUint32(0, true));
    } else {
      document.getElementById(elementId).innerHTML = event.target.value.getFloat32(0,true).toFixed(2);
      console.log("float change: " + event.target.value.getFloat32(0,true));
    }


  }
  getSupportedProperties(characteristic) {
  let supportedProperties = [];
  for (const p in characteristic.properties) {
    if (characteristic.properties[p] === true) {
      supportedProperties.push(p.toUpperCase());
    }
  }
  return '[' + supportedProperties.join(', ') + ']';
}

  _cacheCharacteristic(service, characteristicUuid){
    return service.getCharacteristic(characteristicUuid)
    .then(characteristic => {
      this._characteristics.set(characteristicUuid, characteristic);
    });
  }

 _readCharacteristic(characteristicUuid) {
   let characteristic = this._characteristics.get(characteristicUuid);
   return characteristic.readValue()
   .then(value => {
     value - value.buffer ? value : new DataView(value);
     return value;
   });

 }
 _writeCharacteristic(characteristicUuid, value){
   let characteristic = this._characteristics.get(characteristicUuid);
   return characteristic.writeValue(value);
 }
}

window.env = new Env();

})();
